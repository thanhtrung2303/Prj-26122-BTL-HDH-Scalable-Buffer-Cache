# Cache Statistics Design

- Người phụ trách: Nguyễn Thành Trung.
- Giai đoạn: Thiết kế tuần 2.
- Trạng thái: Đặc tả để triển khai; không phải kết quả đo của mã hiện tại.

## 1. Mục tiêu và phạm vi

Thống kê yêu cầu truy cập buffer cache và lệnh đọc đĩa; phân biệt demand với read-ahead từ Hải, dùng đúng cấu trúc cache và khóa bucket của Quang.

Một đơn vị lookup là một lần gọi đường lấy buffer cho một `(dev, blockno)`, không phải một syscall read, một file hay một lần lặp tìm lại. Mọi loại block đi qua giao diện này đều được tính: dữ liệu và metadata.

Demand ở đây là nguồn mặc định của `bread()` trong kernel, gồm cả đọc metadata và lấy buffer phục vụ ghi. Vì vậy demand không đồng nghĩa chỉ với dữ liệu mà ứng dụng gọi read trực tiếp.

## 2. Nhận biết demand và read-ahead

Định nghĩa chung trong header kernel nhỏ, ví dụ `kernel/cache_stats.h`:

```c
enum cache_source {
  DEMAND = 0,
  READ_AHEAD = 1
};
```


Chốt giao diện ngoài và đường xử lý chung:

```c
struct buf *bread(uint dev, uint blockno);
struct buf *bread_ra(uint dev, uint blockno);
```

- `bread()` gọi hàm chung nội bộ với nguồn `DEMAND`, cấp phát bắt buộc theo hành vi cache nền.
- `bread_ra()` gọi cùng hàm chung với nguồn `READ_AHEAD`, cho phép trả `0` nếu hết buffer rảnh.
- Hàm chung truyền nguồn xuống `bget()` nội bộ và giữ nguồn trong biến cục bộ cho tới chỗ quyết định đọc đĩa.
- Có thể dùng một hàm nội bộ như `bread_source(dev, blockno, source)`; không tạo hai thuật toán cache độc lập.

| Tình huống | Nguồn ghi nhận |
|---|---|
| Block hiện tại mà readi_ra đang trả cho ứng dụng | DEMAND, lấy qua bread |
| Metadata cần để ánh xạ block demand | DEMAND |
| Block dữ liệu đọc trước k+1 hoặc k+2 | READ_AHEAD, lấy qua bread_ra |
| Block indirect được bmap_lookup_ra đọc để phục vụ đọc trước | READ_AHEAD, cũng lấy qua bread_ra |
| Demand đọc lại block đã được read-ahead nạp | DEMAND; thường là lookup hit |

Không đổi toàn bộ hoạt động trong `readi_ra()` thành READ_AHEAD: cùng hàm có cả xử lý demand và đọc trước. Khi OFF, không gọi đường đọc trước, nên không phát sinh sự kiện READ_AHEAD mới sau khi hệ thống đã hết yêu cầu đang chạy.

## 3. Bảng chỉ số

Mỗi bộ đếm là `uint64`. Không lưu các tổng có thể tính lại để tránh cập nhật trùng.

| Tên | Định nghĩa |
|---|---|
| `demand_lookups` | Số lookup DEMAND đã quyết định được kết quả hit/miss |
| `demand_lookup_hits` | DEMAND tìm thấy ánh xạ, ở lần tìm nhanh hoặc tìm lại |
| `demand_lookup_misses` | DEMAND không có ánh xạ và đã cấp phát thành công buffer mới |
| `demand_disk_reads` | Số lệnh đọc đĩa thực sự phát từ nguồn DEMAND |
| `read_ahead_lookups` | Số lookup READ_AHEAD đã quyết định hit/miss/skip |
| `read_ahead_lookup_hits` | READ_AHEAD tìm thấy ánh xạ |
| `read_ahead_lookup_misses` | READ_AHEAD không có ánh xạ và đã cấp phát thành công buffer mới |
| `read_ahead_skips` | READ_AHEAD không có ánh xạ và không còn buffer đủ điều kiện cấp phát |
| `read_ahead_disk_reads` | Số lệnh đọc đĩa thực sự phát từ nguồn READ_AHEAD, gồm metadata |

Tổng và tỷ lệ tính khi hiển thị:

```text
total_lookups = demand_lookups + read_ahead_lookups
total_disk_reads = demand_disk_reads + read_ahead_disk_reads
demand_hit_ratio = demand_lookup_hits / demand_lookups
read_ahead_hit_ratio = read_ahead_lookup_hits / read_ahead_lookups
```

Tỷ lệ READ_AHEAD ở đây tính trên mọi lookup đã có kết quả, gồm cả skip trong mẫu số. Nếu mẫu số bằng 0, hiển thị N/A. Tính phần trăm ở user space, tránh số thực trong kernel.

Không thêm bộ đếm "read_ahead hữu ích" ở phiên bản đầu: các chỉ số trên không theo dõi một block nạp trước có thực sự được demand sử dụng trước khi bị thay thế hay không.

## 4. Định nghĩa lookup hit/miss và skip

### 4.1. Lookup hit

Tìm thấy buffer `mapped == 1` khớp đầy đủ `(dev, blockno)` dưới khóa bucket. Tăng refcnt trước khi nhả khóa rồi mới chờ sleeplock.

Hit không yêu cầu `valid == 1`. Buffer đã ánh xạ nhưng đang chờ nạp dữ liệu vẫn là lookup hit; chỉ kiểm tra valid sau khi lấy sleeplock. Không đọc valid chỉ dưới khóa bucket để phân loại I/O.

### 4.2. Lookup miss

Sau khi lấy eviction lock và toàn bộ khóa bucket theo Quang, tìm lại vẫn không có ánh xạ và cấp phát được buffer mới. Chỉ lúc này mới ghi một miss.

Lần tìm nhanh không thấy chưa được tăng miss ngay. Nếu tiến trình khác đã tạo ánh xạ trước lần tìm lại, yêu cầu được tính hit đúng một lần.

### 4.3. Read-ahead skip

Sau tìm lại vẫn không có ánh xạ, không còn buffer chưa ánh xạ/rảnh để cấp phát: tăng `read_ahead_skips`, trả `0`, không tăng miss và không đọc đĩa cho block đó.

Không dùng chỉ số skip này cho việc bỏ qua vì OFF, chưa đủ tuần tự, EOF hoặc địa chỉ block bằng 0: các trường hợp ấy chưa gọi lấy buffer cho block dữ liệu. Nếu bmap_lookup_ra đã truy cập metadata trước khi phát hiện địa chỉ bằng 0, lookup/I/O metadata đó vẫn được tính.

Demand hết buffer giữ hành vi panic của Quang. Không tạo một kết quả miss giả cho lần cấp phát thất bại; log panic được ghi riêng, không phải một lần đo thành công.

## 5. Vị trí tăng bộ đếm

| Điểm xử lý | Cập nhật |
|---|---|
| bget tìm nhanh thấy ánh xạ | lookups và lookup_hits của nguồn, mỗi trường +1 |
| bget tìm lại thấy ánh xạ sau eviction lock | lookups và lookup_hits của nguồn, mỗi trường +1 |
| bget cấp phát thành công ánh xạ mới | lookups và lookup_misses của nguồn, mỗi trường +1 |
| bget tùy chọn không có buffer | read_ahead_lookups và read_ahead_skips, mỗi trường +1 |
| bread chung đã có sleeplock, valid==0 | disk_reads của nguồn +1 ngay trước virtio_disk_rw(b, 0) |
| bread chung thấy valid==1 | Không tăng disk_reads |
| bwrite/virtio_disk_rw(b, 1) | Không tăng bộ đếm đọc đĩa |
| brelse/bpin/bunpin | Không tăng lookup hoặc disk_reads |

Không tăng lại lookup trong `bread()`, `readi()`, `readi_ra()` hay driver. Driver chỉ nhận I/O; điểm phân loại nguồn và tăng bộ đếm nằm ở hàm bread chung để tránh đếm cả ghi hoặc đếm hai lần.

`disk_reads` đếm lệnh đọc được phát, không phải số lần đọc thành công độc lập với lỗi driver. xv6 hiện không có đường xử lý lỗi I/O có thể khôi phục như hệ thống đầy đủ; nếu thêm retry sau này phải mở rộng đặc tả.

## 6. Cấu trúc lưu thống kê và khóa

Giữ nguyên `struct buf` của Quang: `mapped`, `hnext`, `lastuse`, `refcnt` và các trường I/O. Không thêm trường nguồn yêu cầu vào buffer.

Lưu một mảng thống kê cạnh các bucket trong `kernel/bio.c`:

```c
struct cache_stats {
  uint64 demand_lookups;
  uint64 demand_lookup_hits;
  uint64 demand_lookup_misses;
  uint64 demand_disk_reads;
  uint64 read_ahead_lookups;
  uint64 read_ahead_lookup_hits;
  uint64 read_ahead_lookup_misses;
  uint64 read_ahead_skips;
  uint64 read_ahead_disk_reads;
};

static struct cache_stats bucket_stats[NB_BUCKET];
```

`bucket_stats[i]` được bảo vệ bởi `bcache.bucket[i].lock` hiện có của Quang. Không thêm stats_lock toàn cục. Đây là bộ đếm chia theo bucket để đồng bộ, không phải yêu cầu hiển thị riêng từng bucket.

Ghi nhận lookup theo bucket đích `hash(dev, blockno)`, kể cả khi nạn nhân được chuyển từ bucket khác. Không ghi miss vào bucket nguồn.

Với disk_reads: khi đã giữ buffer sleeplock và refcnt, tính bucket của buffer, lấy khóa bucket trong thời gian ngắn để tăng bộ đếm, nhả khóa rồi mới gọi driver. Refcnt còn dương bảo đảm ánh xạ không đổi trong bước này. Không giữ khóa đồng hồ LRU hoặc eviction lock.

Đường này lấy bucket khi caller đang giữ sleeplock buffer; không tạo vòng khóa vì mọi đường lấy sleeplock từ cache đều phải nhả spinlock trước khi chờ, đúng bất biến của Quang. Không thêm đường nào giữ bucket rồi chờ sleeplock.

Khởi tạo toàn bộ bộ đếm bằng 0 trong binit trước khi có truy cập đồng thời.

## 7. Snapshot, xem và reset

### 7.1. API kernel

Chốt các hàm nội bộ:

```c
void bcache_stats_snapshot(struct cache_stats *out);
void bcache_stats_reset(void);
```

Snapshot và reset lấy toàn bộ khóa bucket theo thứ tự 0 đến 12, thực hiện thao tác trên mảng thống kê, nhả theo thứ tự 12 về 0. Không lấy eviction lock, không lấy lru_clock_lock, không chờ I/O, không copyout hoặc in console khi đang giữ các khóa này.

Snapshot cộng từng trường của 13 bộ vào biến kết quả cục bộ. Reset chỉ xóa mảng thống kê, không đổi cache, refcnt, mapped, valid, lastuse, lru_clock hoặc cấu hình ON/OFF.

Snapshot nhất quán tại mức sự kiện đã được ghi; một yêu cầu có thể đã được ghi lookup nhưng chưa phát I/O. Vì vậy không xem một snapshot là danh sách các yêu cầu đã hoàn tất hoàn toàn.

### 7.2. API user space

Thêm hai syscall theo số trống thực tế trong repo, không hard-code số trong tài liệu:

```c
int cachestat(struct cache_stats *out);
int cachereset(void);
```

- cachestat: chụp vào biến kernel cục bộ, nhả mọi khóa rồi copyout; trả 0 nếu thành công, -1 nếu địa chỉ user không hợp lệ.
- cachereset: xóa bộ đếm dưới khóa; trả 0.
- Định nghĩa struct dùng chung trong header để kernel và user có cùng bố cục.
- Đăng ký syscall và stub ở các file syscall chuẩn của xv6; thêm chương trình `user/cache_stat.c` vào UPROGS.

Giao diện lệnh sau khi triển khai:

```sh
cache_stat
cache_stat reset
```

Lệnh không tham số gọi snapshot và in mọi bộ đếm cùng tỷ lệ; lệnh reset chỉ reset rồi kết thúc. Không gọi snapshot ngầm trước reset. Chương trình phải kiểm tra lỗi syscall, không in số liệu từ vùng nhớ chưa khởi tạo.

### 7.3. Giới hạn đo/reset

Không hủy hoặc đổi nhãn các yêu cầu đang chạy khi reset. Sự kiện lookup trước reset và I/O sau reset có thể nằm ở hai khoảng đo khác nhau. Muốn đo một workload trọn vẹn, dừng workload trước reset, chạy workload đến hoàn tất, rồi lấy snapshot.

Reset không làm cache lạnh. Muốn so sánh cold-cache, khởi động lại QEMU với cùng dữ liệu ảnh đĩa; không gọi reset để giả lập xóa cache. Hoạt động khởi chạy chương trình đo, shell và tiến trình khác cũng có thể tạo lookup, vì thống kê có phạm vi toàn hệ thống, không chỉ một PID. Cần nêu điều này trong báo cáo.

## 8. Ví dụ phân loại

Các ví dụ coi như chỉ có một yêu cầu, không có nhiễu đo; mỗi lookup độc lập được ghi một lần.

| Tình huống | Kết quả lookup | Lệnh đọc đĩa |
|---|---|---|
| Demand block X chưa có; cấp phát được | demand_lookups +1, demand_lookup_misses +1 | demand_disk_reads +1 nếu valid==0 |
| Demand đọc X đã hợp lệ | demand_lookups +1, demand_lookup_hits +1 | Không tăng |
| Hải đọc trước Y chưa có | read_ahead_lookups +1, read_ahead_lookup_misses +1 | read_ahead_disk_reads +1 nếu phát I/O |
| Demand đọc Y sau khi Hải nạp xong | demand_lookups +1, demand_lookup_hits +1 | Không tăng |
| Hải yêu cầu Z nhưng hết buffer | read_ahead_lookups +1, read_ahead_skips +1 | Không tăng |
| Tìm nhanh trượt, tìm lại thấy ánh xạ | lookups +1, lookup_hits +1 theo nguồn | Phụ thuộc valid sau sleeplock |

Hai nguồn cùng yêu cầu một block: nguồn nào thực sự thấy valid==0 và phát I/O nhận disk_reads. Bên còn lại chờ sleeplock rồi thấy valid==1 không được tính thêm đọc đĩa. Không gán I/O theo bên đầu tiên gọi hàm hoặc theo lịch sử buffer.

Nếu lookup hit nhưng buffer đã ánh xạ với valid==0, chính yêu cầu hit có thể phát I/O. Vì vậy không dùng công thức disk_reads = lookup_misses, không khẳng định disk_reads luôn nhỏ hơn số miss.

## 9. Bất biến và kiểm thử

Với bộ đếm chưa tràn, ở mọi snapshot:

```text
demand_lookups = demand_lookup_hits + demand_lookup_misses
read_ahead_lookups = read_ahead_lookup_hits
                   + read_ahead_lookup_misses
                   + read_ahead_skips
```

Để các đẳng thức đúng cả khi reset/snapshot đồng thời, các trường của một kết quả lookup phải được tăng trong cùng critical section của bucket; reset khóa mọi bucket.

| Ca kiểm thử | Tiêu chí |
|---|---|
| Demand miss rồi hit | Phân loại đúng theo ánh xạ, đọc lại buffer valid không phát I/O |
| Read-ahead rồi demand | Lần đầu thuộc READ_AHEAD; lần sau thuộc DEMAND |
| Hai tiến trình cùng miss ban đầu | Chỉ một miss cấp phát; bên tìm lại thấy ánh xạ được tính hit |
| valid==0 và chờ sleeplock | Không đếm hai lệnh đọc cho cùng một lần nạp |
| Read-ahead hết buffer | Có skip, không có miss thành công hoặc I/O cho block bị bỏ |
| Tra cứu indirect cho read-ahead | Metadata mang READ_AHEAD, không bị ghi nhầm DEMAND |
| Ghi đĩa | Không tăng disk_reads tại bwrite |
| Snapshot/reset đồng thời | Không mất đồng bộ, không làm sai cấu trúc cache; đẳng thức lookup giữ đúng |
| Reset | Bộ đếm được xóa tại thời điểm reset; LRU và dữ liệu cache không đổi |
| ON/OFF | Sau khi hết yêu cầu cũ, OFF không tạo thêm READ_AHEAD |

Các số chính xác của ví dụ nên kiểm tra bằng ca kernel được kiểm soát hoặc khoảng đo đã tách nhiễu; không mặc định một lệnh user chỉ phát sinh một lần bread.

## 10. Phối hợp triển khai và cách kết luận

- Quang tích hợp enum nguồn vào đường bget chung, ghi kết quả lookup dưới khóa bucket và giữ nguyên giao thức chống trùng.
- Hải chỉ dùng bread_ra cho block/metadata phục vụ đọc trước; phần demand vẫn dùng bread. Khi nhận 0, dừng cửa sổ hiện tại.
- Trung tích hợp bộ đếm, snapshot/reset, syscall và lệnh xem; giữ độc lập lru_clock trong `timestamp_lru.md`.

Bộ đếm giúp phân tích lookup và chi phí I/O. Demand hit tăng chưa đủ chứng minh read-ahead hiệu quả: phải xem tổng disk_reads, thời gian, tính đúng dữ liệu và cùng cấu hình ON/OFF. Không dùng read_ahead_lookup_hits làm tỷ lệ đọc trước hữu ích và không suy ra contention từ các chỉ số này.

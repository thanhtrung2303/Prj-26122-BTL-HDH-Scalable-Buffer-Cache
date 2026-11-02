# Sequential Read-ahead Design

## 1. Mục tiêu

Read-ahead nạp trước các block có khả năng được đọc tiếp theo vào buffer cache khi chương trình đọc file tuần tự.

Mục tiêu: phát hiện đọc tuần tự theo từng open file description, đọc trước tối đa 2 block bằng I/O đồng bộ và bật/tắt khi biên dịch.

Đây là đặc tả để triển khai. Chưa kết luận read-ahead cải thiện hiệu năng khi chưa có số liệu đo.

## 2. Cơ chế buffer cache hiện tại

Trên bản nền xv6:

- `bget(dev, blockno)` tìm buffer hoặc cấp phát lại một buffer có `refcnt == 0`.
- `bread(dev, blockno)` lấy buffer, đọc đĩa nếu `valid == 0` và trả về buffer đang giữ sleeplock.
- `brelse(b)` nhả sleeplock và giảm `refcnt`.
- `bpin()` và `bunpin()` tăng/giảm số tham chiếu để giữ buffer.

Khi tích hợp, dùng chung thiết kế hash bucket của Quang và timestamp LRU của Trung. Read-ahead không tạo cache riêng, không duy trì danh sách LRU riêng và không thay đổi tổng số buffer `NBUF`.

Đường demand giữ giao diện `bread()` hiện có. Bổ sung đường lấy buffer tùy chọn dành cho read-ahead: dùng cùng bảng băm và giao thức chống trùng, nhưng trả về thất bại nếu không tìm được buffer rảnh, thay vì gọi `panic("bget: no buffers")`.

## 3. Điểm tích hợp

Luồng đọc file:

1. `fileread()` trong `kernel/file.c` nhận yêu cầu đọc.
2. Với file thường, `fileread()` lấy inode lock rồi gọi đường đọc có hỗ trợ read-ahead.
3. Vòng lặp đọc trong `kernel/fs.c` đọc và sao chép dữ liệu demand như trước.
4. Sau mỗi phần dữ liệu sao chép thành công, cập nhật trạng thái tuần tự.
5. Khi đủ điều kiện, nhả buffer demand bằng `brelse()` rồi đọc trước các block tiếp theo.

Giữ nguyên giao diện `readi()` cho các nơi gọi nội bộ. Thêm `readi_ra()` nhận thêm con trỏ trạng thái read-ahead. Hai hàm dùng chung phần xử lý đọc; `readi()` truyền trạng thái rỗng nên không kích hoạt đọc trước. `fileread()` chỉ gọi `readi_ra()` khi inode là file thường `T_FILE`.

Không kích hoạt read-ahead cho pipe, thiết bị, thư mục hoặc những lần đọc inode nội bộ không gắn với một file đang mở.

Trạng thái nằm trong `struct file`, không nằm trong inode: hai lần `open()` có lịch sử đọc độc lập; `dup()` và `fork()` dùng chung `struct file` nên dùng chung offset và lịch sử đọc.

## 4. Phát hiện truy cập tuần tự

### 4.1. Trạng thái

Thêm trạng thái sau vào `struct file`:

| Trường | Kiểu | Ý nghĩa |
|---|---|---|
| `ra_has_history` | `int` | Đã ghi nhận dữ liệu demand đọc thành công hay chưa |
| `ra_next_off` | `uint` | Offset byte ngay sau phần demand đã đọc thành công gần nhất |
| `ra_last_block` | `uint` | Logical block demand gần nhất |
| `ra_seq_blocks` | `uint` | Số logical block liên tiếp của chuỗi, giới hạn tại 2 |

Khởi tạo toàn bộ trường bằng 0 khi tạo file description mới, trước khi công bố file descriptor cho tiến trình. Không khởi tạo lại khi `dup()` hoặc `fork()` chia sẻ cùng file description.

### 4.2. Quy tắc cập nhật

Với mỗi phần demand sao chép thành công, gọi offset bắt đầu là `off`, số byte là `m`, logical block là `bn = off / BSIZE`:

1. Nếu chưa có lịch sử hoặc `off != ra_next_off`, bắt đầu chuỗi mới: `ra_seq_blocks = 1`.
2. Nếu `off == ra_next_off` và vẫn thuộc `ra_last_block`, giữ nguyên số block; đọc nhiều phần nhỏ trong cùng block không được tính thành nhiều block tuần tự.
3. Nếu `off == ra_next_off` và `bn == ra_last_block + 1`, tăng `ra_seq_blocks` nhưng tối đa bằng 2.
4. Các trường hợp còn lại bắt đầu chuỗi mới với giá trị 1.
5. Gán `ra_has_history = 1`, `ra_last_block = bn`, `ra_next_off = off + m`.

Chỉ kích hoạt đọc trước khi đồng thời thỏa mãn:

- `READ_AHEAD_ENABLED == 1`.
- `ra_seq_blocks >= 2`.
- Phần demand vừa đọc kết thúc đúng ranh giới block: `(off + m) % BSIZE == 0`.
- Vẫn còn dữ liệu sau vị trí đó: `off + m < ip->size`.

Áp dụng theo từng block trong vòng lặp, vì vậy một lời gọi `read()` lớn đi qua nhiều block vẫn phát hiện được tuần tự. Block được đọc trước không được cập nhật lịch sử demand.

Ví dụ: đọc hoàn tất block 4 thì số block chuỗi là 1; đọc tiếp và hoàn tất block 5 thì số block chuỗi là 2, kích hoạt đọc trước block 6 và 7. Nếu chỉ đọc nhiều đoạn trong block 4 thì chưa kích hoạt.

### 4.3. Khóa và reset

Với file thường, inode sleeplock `ip->lock` bảo vệ việc đọc/cập nhật trạng thái trên các đường dùng file description đó. `fileread()` hiện đã giữ khóa này trong khi đọc và cập nhật offset; tiếp tục giữ cùng phạm vi khóa khi gọi `readi_ra()`.

Không dùng `ftable.lock` hoặc thêm spinlock để giữ xuyên suốt I/O. Mọi đường truy cập trạng thái sau khi file đã được công bố phải tuân thủ inode lock.

Reset lịch sử khi gặp EOF, lỗi sao chép/đọc hoặc trước khi ghi qua cùng file description dưới inode lock. Đọc 0 byte theo yêu cầu không tạo lịch sử mới. Nếu bổ sung thao tác thay đổi offset trong tương lai, thao tác đó phải reset lịch sử. Đóng rồi mở lại tạo trạng thái mới.

## 5. Số block đọc trước

Cấu hình: `READ_AHEAD_BLOCKS = 2`.

Sau khi demand kết thúc block `k` và đủ điều kiện tuần tự, lần lượt xét logical block `k + 1`, rồi `k + 2`.

- Tối đa xét 2 block cho mỗi lần kích hoạt.
- Không tính block metadata dùng để tra cứu indirect vào cửa sổ 2 block dữ liệu.
- Block đã hợp lệ trong cache không phát sinh lần đọc đĩa mới.
- Không mở rộng cửa sổ thích nghi trong phiên bản đầu.

Đây là cấu hình triển khai ban đầu, chưa phải số block tối ưu đã được chứng minh bằng benchmark.

## 6. Ánh xạ block và EOF

Thêm `bmap_lookup_ra()` trong `kernel/fs.c` để tra cứu địa chỉ block mà không cấp phát:

- Kiểm tra `bn < MAXFILE` và `bn < ceil(ip->size / BSIZE)` trước khi tra cứu; có thể tính giới hạn file bằng `size / BSIZE + (size % BSIZE != 0)`.
- Với direct block, trả về địa chỉ hiện có trong inode.
- Với indirect block, nếu chưa có block indirect thì dừng; nếu có thì đọc metadata qua đường cache tùy chọn dành cho read-ahead, tra cứu địa chỉ rồi `brelse()` ngay.
- Không gọi `balloc()`, không ghi inode, không thay đổi bảng indirect và không dùng `bmap()` có nhánh cấp phát cho các block đọc trước.
- Địa chỉ bằng 0, hết buffer hoặc vượt giới hạn file đều làm dừng cửa sổ đọc trước hiện tại.

Caller giữ inode lock xuyên suốt tra cứu và đọc trước, tránh thay đổi kích thước/ánh xạ inode trong khoảng này. Các đường sửa kích thước/ánh xạ phải tiếp tục tuân thủ inode lock của xv6.

Block cuối file chỉ chứa một phần dữ liệu vẫn có thể được nạp như một block đĩa; số byte trả về cho ứng dụng luôn giới hạn theo `ip->size`. File rỗng hoặc đã đọc tới EOF không kích hoạt read-ahead.

## 7. I/O và đồng bộ

Chốt phiên bản đầu dùng I/O đồng bộ qua `virtio_disk_rw()`. Không tạo worker, hàng đợi hoặc syscall I/O bất đồng bộ.

Giao diện nội bộ cần bổ sung:

| Giao diện | Hợp đồng |
|---|---|
| `bread_ra(dev, blockno)` | Trả buffer giữ sleeplock, dữ liệu hợp lệ; trả `0` khi không cấp phát được buffer rảnh |
| `bmap_lookup_ra(ip, bn)` | Tra cứu không cấp phát, caller giữ inode lock; trả địa chỉ hoặc `0` nếu không có/không thể tra cứu |
| `readi_ra(..., state)` | Đọc demand như `readi()`, cập nhật lịch sử và kích hoạt read-ahead khi đủ điều kiện |

`bread_ra()` dùng chung logic tra cứu/cấp phát với `bread()`, với chế độ cấp phát tùy chọn và nguồn yêu cầu read-ahead. Chỉ khác hành vi khi hết buffer; không sao chép thành một hệ thống cache độc lập.

Luồng đọc trước mỗi block:

1. Tra cứu địa chỉ bằng `bmap_lookup_ra()`.
2. Gọi `bread_ra()`; nếu trả `0` thì dừng cửa sổ đọc trước.
3. Sau khi có sleeplock, kiểm tra `valid`; chỉ đọc đĩa nếu chưa hợp lệ.
4. Sau I/O thành công, đặt `valid = 1`.
5. Gọi `brelse()` đúng một lần cho buffer đã lấy.

Không giữ spinlock bucket, eviction hoặc thống kê khi chờ sleeplock/I/O. Có thể giữ inode sleeplock theo luồng `readi()` của xv6. Không giữ buffer demand trong lúc lấy thêm buffer đọc trước.

Đọc trước đồng bộ có thể làm tăng thời gian của lời gọi hiện tại. Nó không chồng lấp I/O với xử lý ứng dụng; chuyển chờ sang đọc trước không bảo đảm giảm tổng thời gian chạy.

## 8. Bật/tắt

Chốt cấu hình lúc biên dịch trong `kernel/param.h`:

```c
#define READ_AHEAD_ENABLED 1
#define READ_AHEAD_BLOCKS  2
```

- ON: `READ_AHEAD_ENABLED = 1`, cho phép phát hiện tuần tự và đọc trước tối đa 2 block.
- OFF: `READ_AHEAD_ENABLED = 0`, không cập nhật lịch sử, không thực hiện tra cứu/đọc đĩa phục vụ đọc trước.
- Mặc định ON cho bản tích hợp thử nghiệm. Không thêm syscall hoặc lệnh shell trong phiên bản đầu.

Đổi cấu hình rồi biên dịch sạch:

```sh
make clean
make qemu CPUS=3
```

Khi so sánh ON/OFF, giữ nguyên commit triển khai, số CPU, kích thước file, dữ liệu và cấu hình cache. Khởi động lại QEMU giữa các lượt; dùng ảnh đĩa có cùng dữ liệu đầu vào.

## 9. Tránh đọc trùng và tranh chấp

`bread_ra()` tuân thủ cùng giao thức hash bucket:

1. Tìm theo đầy đủ `(dev, blockno)` dưới khóa bucket đích.
2. Nếu đã có, tăng `refcnt` trước khi nhả khóa rồi mới lấy sleeplock. Nếu buffer đang được đọc, chờ sleeplock; sau khi có khóa kiểm tra lại `valid`, không khởi tạo I/O trùng.
3. Nếu chưa có, nhả khóa bucket rồi lấy `eviction_lock` và các khóa bucket theo thiết kế chung của Quang; tìm lại trước khi cấp phát.
4. Ưu tiên buffer chưa ánh xạ; nếu không có, chọn buffer `refcnt == 0` theo timestamp LRU của Trung.
5. Nếu không có buffer đủ điều kiện, nhả mọi spinlock và trả `0`; không đợi buffer rảnh và không panic chỉ vì hết buffer trong đường read-ahead.
6. Nếu cấp phát được, công bố ánh xạ với `refcnt = 1`, `valid = 0` dưới khóa, rồi nhả mọi spinlock trước khi lấy sleeplock và đọc đĩa.

Không tạo hai buffer cho cùng khóa; không thu hồi buffer đang dùng/pin; không giảm `refcnt` thủ công ngoài API cache. Buffer metadata và buffer dữ liệu đều phải được giải phóng sau khi dùng.

Phiên bản đầu dùng cùng quy tắc timestamp của cache: khi `brelse()` hoặc `bunpin()` làm `refcnt` về 0, cập nhật `lastuse` theo `lru_clock` dưới giao thức khóa LRU chung. Read-ahead không cập nhật timestamp lần thứ hai và không có mức ưu tiên thay thế riêng. Không giữ khóa đồng hồ trong lúc chờ I/O.

Về thống kê:

- Mọi yêu cầu cache do đọc trước tạo ra, kể cả tra cứu block indirect, mang nguồn `READ_AHEAD`.
- Demand và metadata phục vụ demand giữ nguồn `DEMAND` theo quy ước thống kê của nhóm.
- Chỉ tăng `read_ahead_disk_reads` tại nơi thực sự phát lệnh đọc đĩa cho nguồn `READ_AHEAD`; cache hit hoặc chờ I/O đã có không tăng bộ đếm này.
- Lookup hit/miss không đồng nghĩa với có/không đọc đĩa. Một buffer đã ánh xạ nhưng `valid == 0` vẫn cần kiểm tra sau khi có sleeplock.
- Một yêu cầu chỉ được tính một kết quả lookup; lần tìm lại sau eviction lock không phải yêu cầu mới. Trường hợp có ánh xạ khi tìm lại được tính hit, cấp phát được buffer mới được tính miss; bỏ qua vì hết buffer cần có chỉ số skip riêng nếu thu thập, không ghi là lần đọc đĩa.

Các tên trường thống kê ngoài `read_ahead_disk_reads` dùng đúng giao diện thống kê chung khi tích hợp, không tạo thêm bộ chỉ số có cùng ý nghĩa.

## 10. Kế hoạch đánh giá

| Ca kiểm thử | Tiêu chí đạt |
|---|---|
| Tuần tự theo block | Chưa đọc trước sau block đầu; sau hai block liên tiếp đã đọc tới ranh giới thì xét hai block kế tiếp |
| Đọc nhỏ hơn block | Nhiều lần đọc trong cùng block không làm tăng sai số block tuần tự |
| Một lần đọc lớn | Phát hiện tuần tự ngay trong vòng lặp của một lời gọi đọc nhiều block |
| Offset không liên tục | Reset chuỗi, không kích hoạt từ lịch sử cũ; có thể kiểm thử nội bộ vì xv6 nền không có lseek chuẩn |
| File rỗng/ngắn/EOF | Không tra cứu ngoài file, không cấp phát block mới; dữ liệu và số byte trả về đúng |
| Direct sang indirect | Ánh xạ đúng; metadata đọc trước được phân loại thống kê đúng |
| Hai open độc lập | Lịch sử không ảnh hưởng lẫn nhau |
| dup/fork dùng chung file | Trạng thái và offset được tuần tự hóa bằng inode lock; không trùng buffer |
| Nhiều tiến trình cùng block | Chỉ một ánh xạ, không đọc đĩa trùng khi buffer đã được nạp |
| Hết buffer rảnh | Đường read-ahead trả bỏ qua, không panic do cấp phát tùy chọn và không rò refcnt |
| ON/OFF | Dữ liệu và số byte trả về giống nhau; OFF không phát I/O đọc trước |
| Ghi rồi đọc lại | So sánh từng byte với dữ liệu đã ghi, không chỉ kiểm tra chương trình chạy xong |

Lưu commit, môi trường, cấu hình, lệnh chạy, trạng thái ON/OFF và log. Ghi số lần đọc đĩa demand/read-ahead, hit/miss và thời gian nếu bộ đếm đã được triển khai.

Đánh giá tổng thời gian cùng tổng số lần đọc đĩa; không suy ra hiệu quả chỉ vì demand hit tăng. Đọc trước có thể làm nóng cache hoặc gây ô nhiễm cache, cần số liệu để kết luận.

## 11. Các quyết định tích hợp đã chốt

| Nội dung | Quyết định trong đặc tả này |
|---|---|
| Trạng thái tuần tự | Theo struct file; cập nhật theo phần demand đã sao chép thành công |
| Ngưỡng kích hoạt | Hai logical block liên tiếp; kích hoạt khi đọc xong ranh giới block |
| Số block đọc trước | Tối đa 2 |
| Điểm tích hợp | fileread -> readi_ra; giữ readi cho bên gọi nội bộ |
| Khóa trạng thái | Inode sleeplock; không giữ spinlock qua I/O |
| Ánh xạ | bmap_lookup_ra không cấp phát |
| I/O | Đồng bộ, không có hàng đợi nền |
| ON/OFF | Macro READ_AHEAD_ENABLED trong kernel/param.h |
| Chống trùng | Chung hash bucket và eviction protocol với demand |
| Thiếu buffer | Dừng read-ahead hiện tại, trả bỏ qua |
| LRU | Chung lru_clock và timestamp của cache |
| Thống kê | Truyền nguồn yêu cầu; tách demand và read-ahead, kể cả metadata |

Phân công triển khai giao diện: Hải phụ trách phát hiện tuần tự, tích hợp tầng file, ánh xạ không cấp phát và test; Quang phụ trách đường cấp phát cache tùy chọn dùng chung với demand; Trung phụ trách timestamp LRU và giao diện thống kê. Các thay đổi bio.c cần đọc chéo trước khi tích hợp.

Những quyết định trên là nội dung đặc tả để nhóm triển khai và review, không phải xác nhận rằng mã hiện tại hoặc toàn bộ kiểm thử đã hoàn thành.
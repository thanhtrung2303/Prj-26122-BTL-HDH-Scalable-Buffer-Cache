# Thiết kế buffer cache dùng hash bucket

- Người phụ trách: Đồng Xuân Quang — 20233603.
- Giai đoạn: Thiết kế tuần 02.
- Trạng thái: Đặc tả đã chốt cho triển khai trong kế hoạch 6 tuần; chưa xác nhận kernel hoặc kiểm thử đã hoàn thành.

## 1. Mục tiêu

Thay việc tìm buffer qua một danh sách dưới khóa toàn cục bằng
bảng băm có khóa riêng cho từng bucket.

Các yêu cầu:
- Mỗi cặp (dev, blockno) có tối đa một buffer đang được ánh xạ.
- Không tái sử dụng buffer có refcnt > 0.
- Không giữ spinlock khi chờ sleeplock hoặc thực hiện I/O.
- Các đường chạy phải tuân thủ cùng một thứ tự lấy khóa.

## 2. Cấu trúc dữ liệu đã chốt

Giữ nguyên tổng số buffer NBUF = 30.

Chốt NB_BUCKET = 13 cho phiên bản đầu. Đây là lựa chọn ban đầu, chưa phải số
bucket tối ưu đã được chứng minh bằng benchmark.

Hàm băm dùng chung:

    h(dev, blockno) = ((uint64)blockno + (uint64)dev) % NB_BUCKET

Chốt tên `bcache.bucket[NB_BUCKET]`; mỗi bucket có:
- Một spinlock `lock`.
- `struct buf *head` là đầu danh sách đơn, nối bằng `hnext`, không phải sentinel LRU.

Giữ `bcache.buf[NBUF]` và `bcache.eviction_lock`. `mapped` có kiểu int, `hnext` là struct buf *, `lastuse` là uint64. Không duy trì danh sách prev/next LRU cũ.

Buffer giữ các trường hiện có cần cho I/O và bổ sung:
- mapped: buffer đã được gán một khóa (dev, blockno) hợp lệ hay chưa.
- hnext: liên kết trong danh sách bucket.
- lastuse: uint64, timestamp logic lần refcnt về 0; dùng lru_clock theo [timestamp_lru.md](timestamp_lru.md).

Dùng thêm eviction_lock để tuần tự hóa các thao tác cache miss
có khả năng cấp phát hoặc thay đổi ánh xạ buffer.

Không sử dụng danh sách LRU toàn cục cũ trong phương án này.

Va chạm hash được xử lý bằng danh sách trong bucket. Khi tìm kiếm,
phải so sánh đầy đủ cả dev và blockno, không chỉ so sánh giá trị hash.

## 3. Khởi tạo

Khi binit:
1. Khởi tạo eviction_lock và các khóa bucket.
2. Khởi tạo sleeplock của từng buffer.
3. Đặt refcnt = 0, valid = 0, mapped = 0, lastuse = 0 và hnext = 0.
4. Khởi tạo lru_clock_lock, đặt lru_clock = 0 và toàn bộ bucket_stats bằng 0.
5. Các danh sách bucket ban đầu rỗng.

Buffer chưa ánh xạ vẫn nằm trong mảng buffer cố định, nhưng chưa
nằm trong danh sách bucket nào.

Trường mapped tránh coi các buffer khởi tạo với dev = 0 và
blockno = 0 là nhiều bản sao của cùng một block.

## 4. Quyền bảo vệ của các khóa

| Khóa | Dữ liệu hoặc thao tác được bảo vệ |
|---|---|
| bucket[i].lock | Danh sách bucket i; ánh xạ và refcnt của buffer đang thuộc bucket i; lastuse và bucket_stats[i] |
| eviction_lock | Tuần tự hóa cấp phát và thay đổi ánh xạ; quản lý buffer chưa ánh xạ |
| b->lock, loại sleeplock | Quyền sử dụng nội dung buffer, kiểm tra valid và đọc/ghi data |
| lru_clock_lock | Đọc/tăng lru_clock; chỉ lấy sau khóa bucket khi cấp timestamp |
| Khóa của driver đĩa | Trạng thái I/O do driver quản lý, giữ theo giao thức hiện tại |

Khi tái sử dụng buffer, việc đặt valid = 0 diễn ra trong giai đoạn
độc quyền: refcnt cũ bằng 0 và các khóa bảo vệ ánh xạ đang được giữ.

lastuse thuộc quyền bảo vệ khóa bucket; lru_clock thuộc lru_clock_lock. Cấp timestamp chỉ khi refcnt từ 1 về 0, dưới cả hai khóa theo thứ tự bucket → lru_clock_lock. Không cần khóa đồng hồ để quét lastuse khi đã giữ mọi bucket.

## 5. Luồng cache hit

1. Tính bucket từ (dev, blockno).
2. Lấy khóa bucket đó.
3. Tìm buffer có mapped = 1 và khớp đầy đủ dev, blockno.
4. Tăng refcnt, đồng thời tăng lookups/hits của nguồn dưới cùng khóa bucket, trước khi nhả khóa.
5. Nhả khóa bucket.
6. Lấy sleeplock của buffer và trả về buffer.

Việc tăng refcnt trước khi nhả khóa ngăn buffer bị thay thế trong
lúc tiến trình đang chờ sleeplock.

Cache hit chỉ cần khóa một bucket và không lấy eviction_lock.

## 6. Luồng cache miss và cấp phát

Phương án đầu tiên ưu tiên tính đúng đắn:

1. Nếu lần tìm đầu tiên không thấy buffer, nhả khóa bucket.
2. Lấy eviction_lock.
3. Lấy lần lượt tất cả khóa bucket theo chỉ số tăng dần.
4. Tìm lại (dev, blockno) trong bucket đích.

Nếu đã có buffer:
- Tăng refcnt và ghi đúng một lookup hit của nguồn. Không ghi miss từ lần tìm nhanh trượt.
- Nhả toàn bộ khóa bucket rồi nhả eviction_lock.
- Sau đó mới lấy sleeplock và trả về buffer.

Nếu vẫn chưa có:
- Ưu tiên buffer mapped = 0, refcnt = 0; chọn chỉ số nhỏ nhất trong bcache.buf nếu có nhiều buffer.
- Nếu không còn, chọn buffer mapped = 1, refcnt = 0 có lastuse nhỏ nhất trên toàn cache; bằng timestamp thì chọn chỉ số mảng nhỏ nhất. Không chọn buffer đầu tiên trong bucket số nhỏ.
- Gỡ buffer khỏi bucket cũ nếu buffer đã được ánh xạ.
- Gán dev, blockno mới; đặt mapped = 1, valid = 0, refcnt = 1.
- Đặt lastuse = 0 khi tạo ánh xạ mới; buffer đang có refcnt = 1 nên không phải nạn nhân.
- Ghi đúng một lookup miss của nguồn dưới khóa bucket đích sau khi cấp phát thành công.
- Chèn buffer vào bucket đích.
- Nhả toàn bộ khóa bucket rồi nhả eviction_lock.
- Sau đó mới lấy sleeplock và trả về buffer.

Nhả các khóa bucket theo chỉ số giảm dần.

Nếu không có buffer đủ điều kiện, quyết định/ghi kết quả dưới khóa như sau; nhả mọi spinlock trước khi trả về hoặc panic:
- DEMAND: giữ hành vi panic("bget: no buffers") của bản nền.
- READ_AHEAD: tăng read_ahead_lookups và read_ahead_skips dưới khóa bucket đích trước khi nhả khóa, sau đó trả 0. Không tăng miss hoặc disk_reads cho khóa bị bỏ, không chờ buffer rảnh.

Phân loại nguồn được truyền từ bread/bread_ra qua hàm chung nội bộ bget_source, không gắn cờ cố định vào struct buf. Nếu skip metadata indirect, Hải phải dừng tra cứu/cửa sổ và giải phóng các buffer đã lấy.

Việc tìm lại sau khi lấy khóa là bắt buộc vì một tiến trình khác
có thể đã tạo ánh xạ trong khoảng giữa hai lần tìm.

## 7. Thứ tự khóa và tránh deadlock

Thứ tự lấy nhiều spinlock:

    eviction_lock -> bucket[0] -> bucket[1] -> ... -> bucket[12] -> lru_clock_lock

Các quy tắc:
- Không lấy eviction_lock trong khi đang giữ khóa bucket.
- Không lấy các khóa bucket theo thứ tự tùy ý.
- Không lấy cùng một khóa bucket hai lần.
- Không lấy bucket hoặc eviction_lock khi đang giữ lru_clock_lock.
- Snapshot/reset thống kê lấy bucket tăng dần, không lấy eviction_lock/lru_clock_lock; không giữ spinlock khi copyout hoặc in console.
- Nhả mọi spinlock trước khi gọi acquiresleep hoặc thực hiện I/O.
- Cache hit, brelse, bpin và bunpin không lấy eviction_lock.

Nếu bucket nguồn và bucket đích trùng nhau, vẫn chỉ giữ khóa
bucket đó một lần; cập nhật danh sách dưới khóa đang giữ.

## 8. brelse, bpin và bunpin

brelse:
1. Nhả sleeplock của buffer.
2. Lấy khóa bucket đang chứa buffer.
3. Giảm refcnt.
4. Chỉ khi refcnt mới bằng 0: lấy lru_clock_lock, gán lastuse = ++lru_clock rồi nhả khóa đồng hồ; vẫn giữ bucket đến bước 5.
5. Nhả khóa bucket.

bpin:
- Lấy khóa bucket, tăng refcnt, rồi nhả khóa.

bunpin:
- Lấy khóa bucket, kiểm tra refcnt > 0 rồi giảm refcnt. Nếu về 0, cập nhật lastuse = ++lru_clock dưới lru_clock_lock trước khi nhả bucket.
- bpin không cập nhật timestamp; brelse/bunpin không di chuyển danh sách hash.
- Phát hiện tràn đồng hồ uint64 theo timestamp_lru.md; không reset đồng hồ khi reset thống kê.

Bên gọi phải giữ một tham chiếu hoặc pin hợp lệ trước các thao tác
này. Vì refcnt chưa bằng 0 trước khi giảm, ánh xạ không thể bị
chuyển sang bucket khác trong lúc xác định khóa cần lấy.

## 9. Các bất biến cần kiểm chứng

1. Một buffer đã ánh xạ xuất hiện trong đúng một bucket.
2. Bucket chứa buffer khớp với hash(dev, blockno).
3. Không có hai buffer đã ánh xạ cùng cặp (dev, blockno).
4. Chỉ thay thế buffer có refcnt = 0.
5. Mọi thay đổi refcnt tuân thủ khóa bảo vệ.
6. Không chờ sleeplock hoặc I/O khi đang giữ spinlock.
7. Mọi buffer trả về từ bget đã có refcnt tăng và sleeplock được giữ.

## 10. Ưu điểm và giới hạn

Ưu điểm:
- Cache hit ở các bucket khác nhau có thể xử lý đồng thời.
- Quy trình cache miss có thứ tự khóa rõ ràng.
- Có thể chọn nạn nhân trên một trạng thái metadata ổn định.

Giới hạn:
- Cache miss tạm khóa tất cả bucket, có thể làm chậm cache hit.
- eviction_lock tuần tự hóa các lần cấp phát.
- Chưa có số liệu chứng minh hiệu năng hoặc số bucket tối ưu.
- Timestamp là thời điểm buffer trở về trạng thái rảnh, không phải LRU theo từng lần bắt đầu truy cập.
- Cấp timestamp còn dùng khóa đồng hồ chung; phải đo chi phí/contention khi đánh giá.

Sau khi phiên bản đầu tiên đúng và có số đo, nhóm có thể đánh giá
phương án chỉ khóa bucket nguồn và đích, kèm kiểm tra lại nạn nhân.

## 11. Quyết định giao tiếp đã chốt

| Nội dung | Quyết định |
|---|---|
| LRU | lru_clock logic; cập nhật ở brelse/bunpin khi refcnt về 0; nạn nhân min lastuse toàn cache, phá hòa theo chỉ số |
| Giao diện | bread(dev, blockno) cho DEMAND; bread_ra(dev, blockno) cho READ_AHEAD; dùng chung bget_source/bread_source nội bộ |
| Nhãn nguồn | enum cache_source theo cache_statistics.md; truyền theo từng yêu cầu, không lưu cố định vào buffer |
| Hit/miss | Tìm lại trúng là một hit; chỉ ghi miss khi cấp phát thành công; không đếm lại lookup trong bread/driver |
| valid và I/O | Hit ánh xạ không đòi valid==1; kiểm tra valid sau sleeplock; chỉ tăng disk_reads của nguồn khi thực sự phát lệnh đọc |
| Metadata đọc trước | bmap_lookup_ra dùng bread_ra cho indirect; không balloc; metadata cũng mang READ_AHEAD |
| Hết buffer | Demand panic; read-ahead skip/return 0, dừng cửa sổ |
| Thống kê | bucket_stats[i] dùng khóa bucket i; snapshot/reset lấy mọi bucket theo thứ tự, không đổi LRU/cache |
| Read-ahead | Đồng bộ tối đa 2 block, ON/OFF lúc biên dịch; buffer được brelse như demand và dùng chung LRU |

Quang phụ trách cấu trúc/cấp phát/cache locking; Trung phụ trách LRU/thống kê/tích hợp; Hải phụ trách tầng file/read-ahead/test dữ liệu. Mốc cụ thể theo [project-plan.md](../project-plan.md), hợp đồng hàm theo [integration.md](integration.md).

Các quyết định trên chốt đặc tả, không tự xác nhận mã hiện tại hoặc test đã đạt. Thiết kế read-ahead mới của Hải cần được cập nhật vào tài liệu repo trong phần bàn giao của Hải.

## 12. Kế hoạch kiểm thử sau triển khai

- Chạy lại usertests và stressfs trên cùng cấu hình baseline.
- Nhiều tiến trình cùng yêu cầu một block đang vắng trong cache.
- Nhiều block khác nhau có cùng giá trị hash.
- Thay thế buffer trong cùng bucket và giữa các bucket.
- Kiểm tra buffer được pin không bị chọn làm nạn nhân.
- Kiểm tra trường hợp không có buffer refcnt = 0.
- Dùng bài kiểm thử có so sánh nội dung để xác nhận dữ liệu.
- Đo contention riêng; không suy ra hiệu năng chỉ từ test hoàn tất.


Ma trận và chủ sở hữu: [tests/test_plan.md](../../tests/test_plan.md). K01–K04/K09 do Quang chính; K05–K08 do Trung chính; K10 phối hợp. Lưu log đúng SHA, không suy ra PASS từ tài liệu.

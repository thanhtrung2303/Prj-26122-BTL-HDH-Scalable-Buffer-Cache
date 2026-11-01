# Thiết kế buffer cache dùng hash bucket

- Người phụ trách: Đồng Xuân Quang — 20233603.
- Giai đoạn: Thiết kế tuần 02.
- Trạng thái: Đề xuất để nhóm review; chưa triển khai vào kernel.

## 1. Mục tiêu

Thay việc tìm buffer qua một danh sách dưới khóa toàn cục bằng
bảng băm có khóa riêng cho từng bucket.

Các yêu cầu:
- Mỗi cặp (dev, blockno) có tối đa một buffer đang được ánh xạ.
- Không tái sử dụng buffer có refcnt > 0.
- Không giữ spinlock khi chờ sleeplock hoặc thực hiện I/O.
- Các đường chạy phải tuân thủ cùng một thứ tự lấy khóa.

## 2. Cấu trúc dữ liệu đề xuất

Giữ nguyên tổng số buffer NBUF = 30.

Chọn thử NB_BUCKET = 13. Đây là lựa chọn ban đầu, chưa phải số
bucket tối ưu đã được chứng minh bằng benchmark.

Hàm băm đề xuất:

    h(dev, blockno) = ((uint64)blockno + (uint64)dev) % NB_BUCKET

Mỗi bucket có:
- Một spinlock.
- Một danh sách các buffer thuộc bucket đó.

Buffer giữ các trường hiện có cần cho I/O và bổ sung:
- mapped: buffer đã được gán một khóa (dev, blockno) hợp lệ hay chưa.
- hnext: liên kết trong danh sách bucket.
- lastuse: timestamp phục vụ chính sách thay thế, chờ thống nhất
  chi tiết với thiết kế timestamp LRU của Trung.

Dùng thêm eviction_lock để tuần tự hóa các thao tác cache miss
có khả năng cấp phát hoặc thay đổi ánh xạ buffer.

Không sử dụng danh sách LRU toàn cục cũ trong phương án này.

Va chạm hash được xử lý bằng danh sách trong bucket. Khi tìm kiếm,
phải so sánh đầy đủ cả dev và blockno, không chỉ so sánh giá trị hash.

## 3. Khởi tạo

Khi binit:
1. Khởi tạo eviction_lock và các khóa bucket.
2. Khởi tạo sleeplock của từng buffer.
3. Đặt refcnt = 0, valid = 0 và mapped = 0.
4. Các danh sách bucket ban đầu rỗng.

Buffer chưa ánh xạ vẫn nằm trong mảng buffer cố định, nhưng chưa
nằm trong danh sách bucket nào.

Trường mapped tránh coi các buffer khởi tạo với dev = 0 và
blockno = 0 là nhiều bản sao của cùng một block.

## 4. Quyền bảo vệ của các khóa

| Khóa | Dữ liệu hoặc thao tác được bảo vệ |
|---|---|
| bucket[i].lock | Danh sách bucket i; ánh xạ và refcnt của buffer đang thuộc bucket i; metadata thay thế theo quy ước chung |
| eviction_lock | Tuần tự hóa cấp phát và thay đổi ánh xạ; quản lý buffer chưa ánh xạ |
| b->lock, loại sleeplock | Quyền sử dụng nội dung buffer, kiểm tra valid và đọc/ghi data |
| Khóa của driver đĩa | Trạng thái I/O do driver quản lý, giữ theo giao thức hiện tại |

Khi tái sử dụng buffer, việc đặt valid = 0 diễn ra trong giai đoạn
độc quyền: refcnt cũ bằng 0 và các khóa bảo vệ ánh xạ đang được giữ.

Timestamp phải có quy tắc khóa rõ ràng khi tích hợp thiết kế LRU.

## 5. Luồng cache hit

1. Tính bucket từ (dev, blockno).
2. Lấy khóa bucket đó.
3. Tìm buffer có mapped = 1 và khớp đầy đủ dev, blockno.
4. Tăng refcnt trước khi nhả khóa bucket.
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
- Tăng refcnt.
- Nhả toàn bộ khóa bucket rồi nhả eviction_lock.
- Sau đó mới lấy sleeplock và trả về buffer.

Nếu vẫn chưa có:
- Ưu tiên buffer chưa ánh xạ.
- Nếu không còn, chọn buffer có refcnt = 0 theo chính sách
  timestamp LRU được nhóm thống nhất.
- Gỡ buffer khỏi bucket cũ nếu buffer đã được ánh xạ.
- Gán dev, blockno mới; đặt mapped = 1, valid = 0, refcnt = 1.
- Khởi tạo metadata thay thế theo quy ước timestamp LRU.
- Chèn buffer vào bucket đích.
- Nhả toàn bộ khóa bucket rồi nhả eviction_lock.
- Sau đó mới lấy sleeplock và trả về buffer.

Nhả các khóa bucket theo chỉ số giảm dần.

Nếu không có buffer đủ điều kiện, giữ hành vi panic
"bget: no buffers" của bản nền trong phương án ban đầu.

Việc tìm lại sau khi lấy khóa là bắt buộc vì một tiến trình khác
có thể đã tạo ánh xạ trong khoảng giữa hai lần tìm.

## 7. Thứ tự khóa và tránh deadlock

Thứ tự lấy nhiều spinlock:

    eviction_lock -> bucket[0] -> bucket[1] -> ... -> bucket[12]

Các quy tắc:
- Không lấy eviction_lock trong khi đang giữ khóa bucket.
- Không lấy các khóa bucket theo thứ tự tùy ý.
- Không lấy cùng một khóa bucket hai lần.
- Nhả mọi spinlock trước khi gọi acquiresleep hoặc thực hiện I/O.
- Cache hit, brelse, bpin và bunpin không lấy eviction_lock.

Nếu bucket nguồn và bucket đích trùng nhau, vẫn chỉ giữ khóa
bucket đó một lần; cập nhật danh sách dưới khóa đang giữ.

## 8. brelse, bpin và bunpin

brelse:
1. Nhả sleeplock của buffer.
2. Lấy khóa bucket đang chứa buffer.
3. Giảm refcnt.
4. Cập nhật timestamp theo quy ước đã thống nhất.
5. Nhả khóa bucket.

bpin:
- Lấy khóa bucket, tăng refcnt, rồi nhả khóa.

bunpin:
- Lấy khóa bucket, giảm refcnt và xử lý timestamp theo quy ước
  đã thống nhất, rồi nhả khóa.

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
- Chất lượng chính sách thay thế phụ thuộc định nghĩa timestamp.

Sau khi phiên bản đầu tiên đúng và có số đo, nhóm có thể đánh giá
phương án chỉ khóa bucket nguồn và đích, kèm kiểm tra lại nạn nhân.

## 11. Nội dung cần thống nhất với nhóm

Với Trung:
- Nguồn timestamp và cách xử lý giá trị bằng nhau.
- Thời điểm cập nhật tại brelse và bunpin.
- Khóa bảo vệ timestamp và thứ tự khóa liên quan.
- Quy tắc đếm hit/miss khi lần tìm đầu trượt nhưng lần tìm lại trúng.

Với Hải:
- Read-ahead phải dùng chung giao thức tra cứu và cấp phát.
- Không tạo đường cấp phát riêng làm trùng buffer.
- Thống nhất hành vi khi read-ahead không còn buffer khả dụng.

## 12. Kế hoạch kiểm thử sau triển khai

- Chạy lại usertests và stressfs trên cùng cấu hình baseline.
- Nhiều tiến trình cùng yêu cầu một block đang vắng trong cache.
- Nhiều block khác nhau có cùng giá trị hash.
- Thay thế buffer trong cùng bucket và giữa các bucket.
- Kiểm tra buffer được pin không bị chọn làm nạn nhân.
- Kiểm tra trường hợp không có buffer refcnt = 0.
- Dùng bài kiểm thử có so sánh nội dung để xác nhận dữ liệu.
- Đo contention riêng; không suy ra hiệu năng chỉ từ test hoàn tất.

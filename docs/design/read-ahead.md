# Sequential Read-ahead Design

## 1. Mục tiêu

Read-ahead nạp trước các block có khả năng được đọc tiếp theo vào buffer cache.
Mục tiêu là giảm số lần phải chờ đọc đĩa khi chương trình truy cập file tuần tự.

Đây là thiết kế đề xuất cho Week 2, chưa phải mô tả một tính năng đã được triển khai.

## 2. Cơ chế buffer cache hiện tại

Mã nguồn hiện tại trong `kernel/bio.c` có các đặc điểm:

- `bget(dev, blockno)` tìm block bằng cách duyệt danh sách buffer.
- `bread(dev, blockno)` lấy buffer và đọc đĩa nếu `valid == 0`.
- `brelse(b)` giải phóng sleeplock, giảm `refcnt` và cập nhật LRU.
- `bpin()` và `bunpin()` điều chỉnh `refcnt`.
- `struct buf` hiện có `valid`, `dev`, `blockno`, `refcnt`, `prev`, `next` và dữ liệu block.

Thiết kế read-ahead phải tương thích với những cơ chế này.

## 3. Điểm tích hợp

Luồng đọc file hiện tại:

1. `fileread()` nhận yêu cầu đọc từ file descriptor.
2. `readi()` trong `kernel/fs.c` xử lý từng block.
3. `bmap()` ánh xạ logical block sang disk block.
4. `bread()` lấy dữ liệu block từ buffer cache hoặc đĩa.
5. `brelse()` giải phóng buffer sau khi sao chép dữ liệu.

Đề xuất:
- Theo dõi tính tuần tự ở tầng đọc file, gắn với từng open file description.
- Tại `readi()`, sau khi xác định block hiện tại, xem xét nạp trước block tiếp theo.
- Không thay đổi thuật toán LRU hoặc cấu trúc buffer cache chỉ để phục vụ read-ahead.

## 4. Phát hiện truy cập tuần tự

Theo dõi logical block cuối cùng được đọc và số lần đọc liên tiếp.

Một lần đọc được xem là tuần tự khi block đầu tiên của lần đọc mới tiếp nối block cuối cùng của lần đọc trước.

Đề xuất trạng thái:
- `last_read_block`: logical block cuối cùng đã đọc.
- `sequential_count`: số lần truy cập liên tiếp theo thứ tự.
- `read_ahead_enabled`: trạng thái bật/tắt read-ahead.

Chỉ kích hoạt prefetch sau khi phát hiện ít nhất hai lần truy cập liên tiếp theo thứ tự. Truy cập ngẫu nhiên sẽ đặt lại bộ đếm tuần tự.

Trạng thái này cần được bảo vệ phù hợp khi nhiều tiến trình dùng chung file description.

## 5. Số block đọc trước

Cấu hình thử nghiệm ban đầu: 2 block tiếp theo.

Ví dụ:
- Tiến trình đọc block 5.
- Khi phát hiện truy cập tuần tự, hệ thống có thể nạp block 6 và 7.
- Nếu block đã nằm trong cache thì không cần đọc lại.

Số lượng 2 là tham số thử nghiệm, không phải giá trị tối ưu đã được đo.

## 6. Ánh xạ block và EOF

Không được dùng trực tiếp `bmap()` để kiểm tra block kế tiếp nếu việc đó có thể cấp phát block mới.

`bmap()` hiện tại có nhánh cấp phát block khi địa chỉ chưa tồn tại. Vì vậy cần một thao tác tra cứu-only, không cấp phát, cho read-ahead.

Các điều kiện:
- Chỉ prefetch block thuộc phạm vi dữ liệu file hiện có.
- Không prefetch khi logical block vượt quá kích thước file.
- Không cấp phát block cho file sparse hoặc vùng chưa được ghi.
- Với indirect block, chỉ tra cứu địa chỉ đã tồn tại.
- Nếu không có block hợp lệ, dừng read-ahead.

## 7. I/O và đồng bộ

`bread()` hiện gọi `virtio_disk_rw()` đồng bộ. Do đó, gọi `bread()` cho block tiếp theo là prefetch đồng bộ, không phải I/O bất đồng bộ.

Phiên bản đầu tiên có thể dùng cơ chế đồng bộ để kiểm tra tính đúng đắn và cache hit. Muốn chồng lấp I/O với xử lý hiện tại cần thiết kế hàng đợi/yêu cầu I/O bất đồng bộ riêng.

Quy tắc:
- Không giữ khóa cache toàn cục trong lúc chờ I/O.
- Dùng `bread()` để lấy buffer nhằm tái sử dụng cơ chế chống trùng block hiện có.
- Sau khi prefetch xong phải gọi `brelse()`.
- Không giữ sleeplock của buffer sau khi kết thúc prefetch.
- Không giảm `refcnt` thủ công.
- Không thay đổi dữ liệu block trong thao tác read-ahead.

## 8. Bật/tắt

Đề xuất có một cấu hình bật/tắt read-ahead để so sánh.

Hai chế độ:
- ON: phát hiện tuần tự và prefetch tối đa 2 block.
- OFF: chỉ đọc các block mà yêu cầu hiện tại cần.

Cơ chế điều khiển cụ thể cần thống nhất với nhóm trước khi thêm syscall hoặc biến cấu hình toàn cục.

## 9. Tránh đọc trùng và tranh chấp

- Nếu block đã có trong cache, tái sử dụng buffer hiện có.
- Không tự tạo một `struct buf` thứ hai cho cùng `(dev, blockno)`.
- Dùng `bread()`/`bget()` hiện có thay vì tự thao tác danh sách buffer.
- Đảm bảo mọi buffer được lấy đều được `brelse()`.
- Không thu hồi buffer đang được tham chiếu hoặc pin.
- Nếu nhiều tiến trình cùng đọc, cơ chế khóa của buffer cache phải bảo vệ dữ liệu.

## 10. Kế hoạch đánh giá

So sánh ON và OFF với:
- Đọc tuần tự.
- Đọc ngẫu nhiên.
- Đọc file ngắn.
- Đọc đến EOF.
- Nhiều tiến trình cùng đọc.

Ghi nhận thời gian chạy, tính đúng dữ liệu và số liệu cache/I/O nếu nhóm đã triển khai bộ đếm.

Không kết luận read-ahead cải thiện hiệu năng nếu chưa có log đo thực tế.

## 11. Các câu hỏi cần thống nhất với nhóm

- Cấu trúc hash bucket và khóa cache có thay đổi không?
- Ai phụ trách thay đổi `bio.c`?
- Có bộ đếm cache hit/miss và số lần đọc đĩa chưa?
- Có API I/O bất đồng bộ hay chỉ dùng `virtio_disk_rw()` đồng bộ?
- Cơ chế bật/tắt read-ahead sẽ được cấu hình ở đâu?

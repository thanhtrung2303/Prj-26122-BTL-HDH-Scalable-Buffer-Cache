# Kế hoạch kiểm thử

Trạng thái: khung kiểm thử, chưa có test riêng của dự án được triển khai.

## Test matrix dự kiến
| Mã | Tình huống | Mục tiêu |
|---|---|---|
| T01 | Build/boot và usertests | Hồi quy chức năng có sẵn |
| T02 | Nhiều tiến trình truy cập cùng block | Dữ liệu đúng, không trùng buffer |
| T03 | Nhiều tiến trình truy cập block khác nhau | Đánh giá contention |
| T04 | Nhiều block hơn dung lượng cache | Kiểm tra thay thế và LRU |
| T05 | Đọc tuần tự bật/tắt read-ahead | Đánh giá thời gian/thông lượng |
| T06 | Đọc ngẫu nhiên bật/tắt read-ahead | Đánh giá tác dụng phụ |
| T07 | Đọc/ghi phối hợp, pin/unpin | Hồi quy, dữ liệu đúng |
| T08 | Tình huống hit/miss có kiểm soát | Kiểm tra định nghĩa/bộ đếm |

Mỗi test bổ sung: chủ sở hữu, dữ liệu đầu vào, lệnh chạy, kết quả mong đợi, kết quả thực tế, commit và link log.
Không mặc định repo có bcachetest hoặc các bài lab khác; kiểm tra trước khi dùng.

Chương trình C chạy trong xv6 đặt ở user/ và thêm UPROGS trong Makefile.
Script điều khiển từ host đặt ở scripts/.

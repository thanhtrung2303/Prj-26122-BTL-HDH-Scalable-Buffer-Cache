# Kế hoạch kiểm thử Buffer Cache

**Tác giả:** NguyenXuanHai202233383

## 1. Kiểm thử tính đúng đắn

- Đọc cùng một khối nhiều lần và xác minh dữ liệu nhận được.
- Ghi dữ liệu rồi đọc lại để kiểm tra tính nhất quán.
- Truy cập nhiều khối khác nhau và xác minh nội dung giữa các khối không bị lẫn.
- Kiểm tra việc loại bỏ bộ đệm và nạp lại dữ liệu, bảo đảm dữ liệu vẫn chính xác.

## 2. Kiểm thử truy cập đồng thời

- Cho nhiều tiến trình đọc các khối khác nhau cùng lúc.
- Cho nhiều tiến trình truy cập cùng một khối.
- Kiểm tra xem có xuất hiện nhiều mục bộ đệm cho cùng một thiết bị và số hiệu khối hay không.
- Kiểm tra bộ đếm tham chiếu, việc sử dụng khóa và tính toàn vẹn của danh sách bộ đệm.
- Chạy các bài kiểm thử sẵn có của dự án và ghi lại lỗi nếu có.

## 3. Kiểm thử đọc trước dữ liệu tuần tự (Read-ahead)

- Đọc một chuỗi các khối liên tiếp.
- Nếu dự án hỗ trợ bật/tắt Read-ahead, so sánh hành vi trong hai trường hợp.
- Xác minh dữ liệu ở đầu và cuối chuỗi đọc.
- Kiểm tra Read-ahead có giảm số lần đọc theo yêu cầu hay không, đồng thời tránh phát sinh I/O dư thừa.

## 4. Đo lường hiệu năng

Ghi lại các thông tin sau trong mỗi lần chạy:

| Chỉ số | Kết quả hiện tại |
|---|---|
| Tên bài kiểm thử | Chưa chạy |
| Số lần truy cập trúng bộ đệm (cache hit) | Chưa đo |
| Số lần truy cập trượt bộ đệm (cache miss) | Chưa đo |
| Thời gian thực hiện | Chưa đo |
| Tính đúng đắn | Chưa xác minh |

Không ghi số liệu ước tính như thể đó là kết quả đo thực tế.

## 5. Môi trường kiểm thử

Ghi lại phiên bản mã nguồn xv6, cấu hình biên dịch, lệnh chạy kiểm thử và các thiết lập trình giả lập có liên quan. Những thông tin này giúp người khác có thể chạy lại và đối chiếu kết quả.

## 6. Danh sách công việc cần kiểm tra

- [ ] Đọc và rà soát phần triển khai Buffer Cache hiện có.
- [ ] Xác định các bài kiểm thử sẵn có trong dự án.
- [ ] Chạy kiểm thử tính đúng đắn.
- [ ] Chạy kiểm thử truy cập đồng thời.
- [ ] Đánh giá cơ chế đọc trước dữ liệu tuần tự.
- [ ] Ghi lại kết quả thực tế và các vấn đề phát hiện được.
- [ ] Trao đổi, rà soát kết quả với các thành viên trong nhóm.

## 7. Trạng thái kiểm thử

Đây là kế hoạch kiểm thử. Chỉ đánh dấu hoàn thành các mục sau khi đã thực hiện và xác minh; hiện chưa có số liệu thực nghiệm được ghi nhận trong tài liệu này.

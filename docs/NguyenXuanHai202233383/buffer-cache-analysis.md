# Phân tích thiết kế Buffer Cache

**Tác giả:** NguyenXuanHai202233383

## 1. Mục tiêu

Bộ đệm khối (Buffer Cache) lưu tạm dữ liệu của các khối trên thiết bị lưu trữ để hạn chế việc đọc thiết bị nhiều lần. Thiết kế **Scalable Buffer Cache** cần giữ nguyên tính đúng đắn, đồng thời giảm tranh chấp khi nhiều tiến trình hoặc CPU truy cập các khối dữ liệu khác nhau.

Các mục tiêu cần xem xét:

- Xác định chính xác khối dữ liệu đang được lưu trong bộ đệm.
- Không tạo nhiều bộ đệm cho cùng một cặp thiết bị và số hiệu khối.
- Cho phép các thao tác độc lập diễn ra đồng thời khi an toàn.
- Bảo đảm khóa bộ đệm và bộ đếm tham chiếu được quản lý đúng.
- Đo lường số lần truy cập trúng/trượt bộ đệm và hiệu năng.

## 2. Tổ chức theo nhóm băm (Hash Buckets)

Hàm băm có thể sử dụng thông tin thiết bị và số hiệu khối để xác định nhóm băm chứa bộ đệm tương ứng. Mỗi nhóm quản lý danh sách các bộ đệm thuộc nhóm đó.

Dùng khóa riêng cho từng nhóm có thể giảm tranh chấp so với việc mọi thao tác đều phải lấy một khóa toàn cục. Tuy nhiên, thiết kế phải bảo đảm rằng các thao tác đồng thời không làm hỏng danh sách hoặc tạo ra bản sao bộ đệm.

Những điểm cần bảo đảm:

- Tìm bộ đệm đã tồn tại trước khi cấp phát bộ đệm mới.
- Ngăn hai yêu cầu đồng thời tạo hai bộ đệm cho cùng một thiết bị và khối dữ liệu.
- Bảo vệ danh sách bộ đệm và siêu dữ liệu bằng các khóa phù hợp.
- Quy định thứ tự lấy khóa an toàn khi chuyển bộ đệm giữa các nhóm.
- Xử lý trường hợp bộ đệm đã dùng hết mà không làm mất hoặc nhân đôi dữ liệu.

Các lựa chọn triển khai cuối cùng cần được đối chiếu với mã nguồn thực tế và quy tắc khóa của dự án.

## 3. Chính sách LRU dựa trên dấu thời gian

LRU (Least Recently Used) là chính sách ưu tiên loại bỏ bộ đệm ít được sử dụng gần đây nhất. Với cách tiếp cận dựa trên dấu thời gian, hệ thống ghi lại thời điểm bộ đệm được sử dụng hoặc được giải phóng gần nhất theo quy ước mà dự án lựa chọn.

Khi cần một bộ đệm để lưu khối mới, hệ thống có thể chọn bộ đệm đủ điều kiện sử dụng lại có dấu thời gian cũ nhất.

Cần chú ý:

- Không loại bỏ bộ đệm đang bận hoặc vẫn còn được tham chiếu.
- Cập nhật dấu thời gian theo một quy tắc nhất quán.
- Đồng bộ hóa các cập nhật đồng thời đối với dấu thời gian và bộ đếm tham chiếu.
- Kiểm tra lại tính hợp lệ của bộ đệm được chọn trước khi tái sử dụng.

## 4. Đọc trước dữ liệu tuần tự (Sequential Read-ahead)

Khi chương trình đọc các khối liên tiếp nhau, hệ thống có thể yêu cầu đọc một khối tiếp theo trước khi chương trình thực sự cần khối đó. Nếu dự đoán đúng, dữ liệu có thể đã sẵn sàng trong bộ đệm và giúp giảm thời gian chờ đọc.

Cần đánh giá các vấn đề sau:

- Phát hiện việc truy cập các khối liên tiếp.
- Tránh gửi nhiều yêu cầu đọc đồng thời cho cùng một khối.
- Xử lý đúng điểm kết thúc tệp và phạm vi khối không hợp lệ.
- Tránh đọc thừa quá nhiều dữ liệu khi chương trình truy cập không tuần tự.
- Không làm sai lệch quy tắc khóa hoặc tính đúng đắn của bộ đệm.

Read-ahead trong tài liệu này là nội dung phân tích đề xuất; chỉ ghi nhận là đã triển khai sau khi có thay đổi mã nguồn và kết quả kiểm thử tương ứng.

## 5. Tiêu chí đánh giá

Có thể đánh giá thiết kế dựa trên tính đúng đắn của dữ liệu, khả năng xử lý truy cập đồng thời, mức độ tranh chấp khóa, số lần truy cập thiết bị lưu trữ và hiệu năng ở các kiểu đọc khác nhau.

Các kết quả đo thực tế cần được ghi lại riêng, kèm môi trường và cách chạy kiểm thử để có thể tái hiện.

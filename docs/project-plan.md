# Kế hoạch 9 tuần

Kế hoạch dự kiến; cập nhật theo quyết định nhóm và lịch giảng viên. Chưa gán ngày bắt đầu/kết thúc chính thức.

| Tuần | Công việc | Điều kiện đạt mốc |
|---|---|---|
| 1 | Xác nhận môi trường, khảo sát, thiết lập công cụ | Có đầu ra và báo cáo trung thực |
| 2 | Chốt thiết kế, đồng bộ và test matrix | Cả nhóm review thiết kế trước khi sửa lõi |
| 3 | Hash bucket/khóa từng bucket | Build, boot, test cơ bản; kiểm tra trùng buffer |
| 4 | Timestamp LRU/đồng thời | Tái sử dụng đúng, không deadlock trong các test đã chạy |
| 5 | Hit/miss và đo ban đầu | Có định nghĩa, bộ đếm và log |
| 6 | Read-ahead | Đủ chức năng chính; bật/tắt được; dữ liệu đúng |
| 7 | Đánh giá tổng thể | Số liệu so sánh, phân tích và sửa lỗi |
| 8 | Báo cáo/slide/ổn định | Bản thảo và demo hoàn chỉnh |
| 9 | Dự phòng/luyện bảo vệ | Tái hiện kết quả và bàn giao |

## Phân công dự kiến sau tuần 1
- Quang: hash bucket và đồng bộ.
- Trung: timestamp LRU, thống kê, phối hợp tích hợp.
- Hải: read-ahead, chương trình kiểm thử và tổng hợp số liệu.
- Cả nhóm: review chéo, phân tích kết quả, báo cáo và bảo vệ.

Phân công cần được nhóm xác nhận trước triển khai.

## Quản lý
Trello: Backlog → Tuần này → Đang làm → Chờ kiểm tra → Hoàn thành.
Mỗi thẻ có người phụ trách, hạn, checklist, điều kiện hoàn thành và link đầu ra.
Mỗi tuần chốt việc đã làm, vấn đề, nhiệm vụ kỳ tới và milestone tiếp theo.

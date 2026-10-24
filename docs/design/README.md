# Thiết kế và sơ đồ

Figma/FigJam: **Chưa bổ sung**.

Tuần 1: sơ đồ tổ chức cache và luồng đọc hiện tại, ghi commit nguồn.
Tuần 2: thiết kế hash bucket, bảng khóa/dữ liệu được bảo vệ, thứ tự lấy khóa, tái sử dụng buffer, timestamp LRU, read-ahead và bộ đếm.

Các điều kiện cần bảo đảm: không có hai buffer cho cùng (dev, blockno); không thu hồi buffer đang được dùng hoặc pin; dữ liệu được đồng bộ đúng; tránh deadlock.

Lưu ảnh xuất từ sơ đồ vào thư mục này; ghi nguồn và phân biệt thiết kế hiện tại với thiết kế đề xuất.

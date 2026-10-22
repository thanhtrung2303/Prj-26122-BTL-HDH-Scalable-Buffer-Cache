# Khảo sát luồng buffer cache hiện tại

**Phụ trách: Trung. Trạng thái: khung chưa điền.**

## Struct buf
Lập bảng trường dữ liệu, kiểu, ý nghĩa và cơ chế đồng bộ liên quan. Đối chiếu đúng kernel/buf.h trong commit đang dùng.

| Trường | Ý nghĩa | Được bảo vệ/cập nhật thế nào? |
|---|---|---|

## Luồng xử lý
Đọc bget(), bread(), brelse(); tìm hiểu thêm vai trò bwrite(), bpin(), bunpin().
- Trường hợp tìm thấy buffer và dữ liệu hợp lệ.
- Trường hợp tìm thấy buffer nhưng dữ liệu chưa hợp lệ.
- Trường hợp cần tái sử dụng buffer.
- Vai trò refcnt, khóa cache và khóa buffer.
- Ảnh hưởng pin/unpin đến điều kiện tái sử dụng.

## Sơ đồ và đối chiếu
- Commit đã đọc:
- Link Figma/FigJam:
- Hàm/file làm căn cứ:
- Câu hỏi còn tồn tại:

Đầu ra mong đợi khoảng 1–2 trang, được một thành viên khác đọc lại.

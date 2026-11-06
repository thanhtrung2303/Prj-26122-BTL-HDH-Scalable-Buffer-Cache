# Kết quả kiểm thử bản nền của Quang

- Người thực hiện: Đồng Xuân Quang — 20233603.
- Ngày kiểm thử: 2026-11-01.
- Commit nền: 59f4752; mã đầy đủ trong quang-environment.txt.
- Nhánh: docs/quang-week02-design-baseline.
- Cấu hình QEMU: 3 CPU ảo, RAM 128 MiB.
- Môi trường chi tiết: quang-environment.txt.
- Chạy trên fs.img hiện có; không tạo lại ảnh đĩa giữa hai test.
- Thứ tự: usertests, thoát QEMU, khởi động lại rồi chạy stressfs.

## Kết quả

| Kiểm thử | Lệnh trong xv6 | Kết quả |
|---|---|---|
| Bộ kiểm thử hệ thống | usertests | ALL TESTS PASSED |
| Truy cập hệ thống file đồng thời | stressfs | Hoàn tất và trở lại shell; không quan sát thấy panic hoặc treo |

## Minh chứng

- [Môi trường](quang-environment.txt)
- [Log usertests](quang-usertests-cpus3.log)
- [Log stressfs](quang-stressfs-cpus3.log)

## Nhận xét

Các thông báo hết block và hết inode xuất hiện trong những test
cố tình kiểm tra tình huống cạn tài nguyên. Các test tương ứng
báo OK và toàn bộ usertests kết thúc với ALL TESTS PASSED.

Trong stressfs, các dòng write có ký tự xen kẽ do nhiều tiến trình
cùng in ra console. Chương trình hoàn tất và shell nhận lệnh trở lại.

stressfs hiện tại không so sánh nội dung đọc với dữ liệu đã ghi,
nên kết quả này chưa chứng minh toàn bộ tính đúng đắn dữ liệu.

Đây là kết quả của một lần chạy mỗi chương trình trên cấu hình
đã ghi. Chưa đo contention, throughput hoặc hiệu quả read-ahead.

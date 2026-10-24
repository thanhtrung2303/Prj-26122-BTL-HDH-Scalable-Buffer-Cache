# Môi trường và hướng dẫn chạy

**Phụ trách: Quang. Trạng thái: khung cần xác nhận bằng thực nghiệm.**

## Thông tin cần ghi
- Hệ điều hành/WSL:
- Nguồn mã và commit:
- Phiên bản GCC RISC-V:
- Phiên bản QEMU (Makefile yêu cầu >= 7.2):
- Phiên bản Make:
- Thay đổi từ Tutorial 4 nếu dùng lại môi trường:

## Kiểm tra
Trong terminal host: chạy `git rev-parse HEAD`, `qemu-system-riscv64 --version`, `make qemu`.
Trong xv6: chạy `ls`, `cat README`; ghi kết quả thực tế.
Thoát: Ctrl+A rồi X.

Không chạy đồng thời hai QEMU cùng ghi vào một fs.img.
Chỉ dùng `make clean` khi chấp nhận tạo lại ảnh đĩa thử.

## Kết quả xác nhận
| Người | Ngày | Commit | Build/boot | Lỗi còn tồn tại | Minh chứng |
|---|---|---|---|---|---|

## Lỗi và cách xử lý
Ghi thông báo lỗi, nguyên nhân đã xác định, cách xử lý và kết quả chạy lại.

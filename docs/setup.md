# Môi trường và hướng dẫn chạy xv6

Người thực hiện: Đồng Xuân Quang — MSSV 20233603.

Trạng thái: Quang đã build, boot và kiểm tra thao tác file cơ bản.
Đang chờ Trung chạy lại theo hướng dẫn và xác nhận.

## 1. Môi trường đã kiểm tra

| Thành phần | Phiên bản |
|---|---|
| Hệ điều hành | Ubuntu 26.04.1 LTS |
| Kernel Linux của máy host | 7.2.6 |
| Kiến trúc máy host | x86_64 |
| QEMU | 10.2.1 |
| GCC RISC-V | riscv64-linux-gnu-gcc 15.2.0 |
| GNU Make | 4.4.1 |

Thông tin chi tiết được lưu tại:
[Log môi trường](../reports/week-01/evidence/quang-environment.txt).

Các phiên bản trên là môi trường đã chạy thử, không phải yêu cầu
tất cả thành viên phải dùng cùng phiên bản kernel Ubuntu.

## 2. Mã nguồn

- Repo: https://github.com/thanhtrung2303/Prj-26122-BTL-HDH-Scalable-Buffer-Cache.git
- Nhánh làm việc: docs/quang-setup-cache
- Commit mã nguồn đã kiểm tra: 5f9de937b91cf3b3e112c41e18bc2f989af7b35d
- Mã nguồn xv6-riscv nằm ngay tại thư mục gốc của repo.
- Chưa chỉnh sửa chức năng kernel trong quá trình kiểm tra này.
- Bản Tutorial 4 cũ: chưa đối chiếu nguồn, commit và các thay đổi;
  cần bổ sung nếu nhóm quyết định sử dụng lại.

## 3. Cài công cụ trên Ubuntu

Chạy trong Terminal Ubuntu:

```bash
sudo apt update
sudo apt install git build-essential perl python3 bc \
  qemu-system-misc gcc-riscv64-linux-gnu \
  binutils-riscv64-linux-gnu gdb-multiarch
```

Kiểm tra:

```bash
qemu-system-riscv64 --version
riscv64-linux-gnu-gcc --version
make --version
```

Makefile hiện tại yêu cầu QEMU từ 7.2 trở lên.

## 4. Lấy mã nguồn

Nếu chưa có repo:

```bash
mkdir -p ~/Projects
cd ~/Projects
git clone https://github.com/thanhtrung2303/Prj-26122-BTL-HDH-Scalable-Buffer-Cache.git
cd Prj-26122-BTL-HDH-Scalable-Buffer-Cache
```

Ghi lại phiên bản đang dùng:

```bash
git branch --show-current
git rev-parse HEAD
git status --short
```

## 5. Build và chạy

Tại thư mục gốc của repo:

```bash
make qemu
```

Để lưu log cho một lần kiểm tra mới:

```bash
mkdir -p reports/week-01/evidence
script -q -c "make qemu" reports/week-01/evidence/build-boot-new.log
```

Dùng tên log khác cho mỗi lần cần giữ riêng minh chứng.

Khởi động thành công khi xuất hiện:

```text
xv6 kernel is booting
...
init: starting sh
$
```

Cấu hình lần chạy đã kiểm tra: 3 CPU ảo, RAM 128 MiB.

Thoát QEMU: nhấn Ctrl+A, thả phím, sau đó nhấn X.

Không chạy đồng thời hai QEMU cùng sử dụng một fs.img.
Lệnh make clean xóa sản phẩm build và fs.img, làm mất dữ liệu
thử nghiệm được tạo trong ảnh đĩa.

## 6. Kiểm tra trong shell xv6

Các lệnh dưới đây chạy tại dấu nhắc $ của xv6:

```sh
ls
cat README
mkdir quangtest
echo quang-xv6-ok > quangtest/check.txt
cat quangtest/check.txt
ls quangtest
```

Nếu chạy lại trên cùng ảnh đĩa và quangtest đã tồn tại,
bỏ qua lệnh mkdir hoặc dùng tên thư mục thử khác.

| Thao tác | Kết quả quan sát |
|---|---|
| Khởi động xv6 | Vào được shell |
| ls | Hiển thị danh sách file |
| cat README | Đọc được nội dung README |
| mkdir quangtest | Tạo được thư mục |
| echo ghi vào check.txt | Ghi được nội dung |
| cat quangtest/check.txt | In ra quang-xv6-ok |
| ls quangtest | Hiển thị check.txt |

Đây là kiểm tra build, boot và thao tác file cơ bản.
Chưa chạy usertests, stressfs hoặc đo hiệu năng buffer cache.

## 7. Minh chứng

- [Log build và chạy](../reports/week-01/evidence/quang-build-boot.log)
- [Thông tin môi trường](../reports/week-01/evidence/quang-environment.txt)

Ảnh chụp khởi động và thao tác file sẽ được bổ sung vào thư mục
reports/week-01/evidence/.

## 8. Kết quả xác nhận

| Người | Ngày | Kết quả | Trạng thái |
|---|---|---|---|
| Quang | 2026-10-24 | Build, boot, đọc và ghi file cơ bản thành công | Đã tự kiểm tra |
| Trung | Chưa xác nhận | Chưa chạy lại theo hướng dẫn | Chờ đọc chéo |

## 9. Vấn đề còn tồn tại

- Chưa đối chiếu với bản Tutorial 4 cũ.
- Chưa có xác nhận chạy lại từ thành viên khác.
- Chưa phát hiện lỗi chặn build/boot trong lần kiểm tra đã ghi nhận.

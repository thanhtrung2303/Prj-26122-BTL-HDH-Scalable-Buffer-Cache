# Project 26122 — Scalable buffer cache with read-ahead

Bài tập lớn môn **Hệ điều hành**, học kỳ **20261**. Dự án cải tiến buffer cache trên **xv6-riscv** để giảm tranh chấp khóa và đánh giá hiệu quả đọc trước khi truy cập tuần tự.

> Trạng thái: khởi tạo tài liệu và cấu trúc dự án. Các chức năng cải tiến, kết quả kiểm thử và tỷ lệ tiến độ chỉ được cập nhật khi có minh chứng. Kế hoạch thực hiện: **9 tuần**.

## Thành viên

| Thành viên | MSSV | Trách nhiệm tuần 1 |
|---|---|---|
| Đồng Xuân Quang | 20233603 | Kiểm tra môi trường, hướng dẫn chạy, khảo sát tổ chức cache |
| Nguyễn Thành Trung | 20233680 | Quản lý repo, khảo sát luồng buffer cache, tổng hợp báo cáo |
| Nguyễn Xuân Hải | 20233383 | Phân tích yêu cầu, tìm hiểu khái niệm, Trello và kế hoạch |

Đây là phân công tuần 1. Phân công triển khai các tuần sau được nhóm chốt theo kế hoạch và công việc thực tế.

## Mục tiêu và phạm vi

- Tổ chức buffer cache thành hash bucket với khóa riêng cho từng bucket.
- Sử dụng timestamp LRU để chọn buffer đủ điều kiện tái sử dụng.
- Bổ sung sequential read-ahead.
- Thống kê cache hit/miss với định nghĩa đếm rõ ràng.
- Kiểm tra tính đúng đắn, tranh chấp khóa, thông lượng và hiệu quả read-ahead.
- Demo contention trước/sau và tốc độ đọc tuần tự khi tắt/bật read-ahead.

**Nâng cao:** write-back flusher; cấu hình dung lượng cache khi khởi động. Chỉ triển khai khi phần bắt buộc đã ổn định.

## Sản phẩm bàn giao

1. Mã nguồn xv6 đã cải tiến, trọng tâm là `kernel/bio.c` và các file liên quan.
2. Tài liệu yêu cầu, thiết kế, hướng dẫn build/chạy và quy trình đóng góp.
3. Chương trình/kịch bản kiểm thử; log gốc, cấu hình chạy và bảng kết quả so sánh.
4. Báo cáo tiến độ theo mẫu giảng viên, báo cáo cuối kỳ và slide.
5. Kịch bản demo có thể chạy lại.

Các sản phẩm chung khác cần đối chiếu thêm với phần Overview và thông báo của giảng viên.

## Cấu trúc repo

| Đường dẫn | Nội dung |
|---|---|
| `kernel/` | Mã nguồn kernel xv6; buffer cache ở `bio.c`, buffer ở `buf.h` |
| `user/` | Chương trình chạy trong xv6; thêm chương trình test tại đây và đăng ký trong `UPROGS` |
| `mkfs/` | Công cụ tạo ảnh hệ thống file |
| `docs/` | Yêu cầu, khảo sát, thiết kế, cài đặt, kế hoạch |
| `docs/design/` | Thiết kế và sơ đồ; link Figma/FigJam |
| `tests/` | Test matrix và mô tả cách chạy các tình huống |
| `scripts/` | Script phía máy host phục vụ chạy thử và tổng hợp |
| `results/` | Log gốc và kết quả đo đã kiểm chứng |
| `reports/week-01/` | Khung báo cáo và minh chứng tuần 1 |
| `reports/final/` | Báo cáo cuối kỳ |
| `slides/` | Slide trình bày và kịch bản demo |
| `README` | Hướng dẫn và ghi nhận tác giả xv6 gốc |
| `LICENSE` | Giấy phép mã nguồn gốc |

## Lấy mã nguồn và chạy

Thực hiện trong Ubuntu hoặc Ubuntu trên WSL2. VS Code dùng để đọc/sửa code; build và chạy bằng terminal Linux.

```bash
git clone https://github.com/thanhtrung2303/Prj-26122-BTL-HDH-Scalable-Buffer-Cache.git
cd Prj-26122-BTL-HDH-Scalable-Buffer-Cache
make qemu
```

Cần Git, Make, GCC cho host, Perl, `bc`, bộ GCC/binutils RISC-V và `qemu-system-riscv64`. Makefile hiện tại yêu cầu **QEMU >= 7.2**.

Ví dụ cài gói trên Ubuntu có cung cấp các gói tương ứng:

```bash
sudo apt update
sudo apt install git build-essential perl python3 bc qemu-system-misc gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf
qemu-system-riscv64 --version
```

Quang xác nhận cách cài trên môi trường thực tế và cập nhật [setup.md](docs/setup.md). Không coi ví dụ cài gói là kết quả đã kiểm chứng trên mọi phiên bản Ubuntu.

Trong shell xv6, thử `ls`, `cat README`. Thoát QEMU bằng **Ctrl+A**, thả phím, rồi nhấn **X**.

`make clean` xóa sản phẩm build và `fs.img`, làm mất dữ liệu thử đã tạo trong ảnh đĩa. Chỉ dùng khi cần khởi tạo lại môi trường.

## Kế hoạch 9 tuần

| Tuần | Công việc | Mốc đầu ra |
|---|---|---|
| 1 | Môi trường, khảo sát cache, tổ chức dự án | Tài liệu khảo sát và báo cáo tiến độ |
| 2 | Khảo sát sâu và chốt thiết kế | Bucket, quy tắc khóa, LRU, hướng read-ahead, test matrix |
| 3 | Hash bucket và khóa từng bucket | Bản mới vượt kiểm thử cơ bản |
| 4 | Timestamp LRU và kiểm tra đồng thời | Tái sử dụng buffer đúng, xử lý lỗi đồng bộ |
| 5 | Hit/miss và đo ban đầu | Bộ đếm, bài test, số liệu bước đầu |
| 6 | Tích hợp read-ahead | Đủ chức năng bắt buộc, bật/tắt được để so sánh |
| 7 | Kiểm thử tổng thể và hiệu năng | Log, bảng so sánh và sửa lỗi |
| 8 | Ổn định, báo cáo và slide | Bản demo và bản thảo đầy đủ |
| 9 | Dự phòng, hoàn thiện và luyện bảo vệ | Bộ bàn giao và demo chạy lại được |

Chi tiết: [kế hoạch](docs/project-plan.md), [phân công tuần 1](docs/week-01-tasks.md).

## Quy trình đóng góp

1. Chọn công việc trên Trello, xác định đầu ra và người kiểm tra.
2. Cập nhật `main`, tạo nhánh cho nhiệm vụ.
3. Sửa file, kiểm tra và commit đúng những file liên quan.
4. Push nhánh, mở PR về `main`.
5. Một người khác đọc lại; tác giả sửa theo góp ý rồi nhóm merge.
6. Gắn link PR/minh chứng vào Trello, cập nhật trạng thái.

```bash
git switch main
git pull --ff-only origin main
git switch -c docs/buffer-cache-notes
# Sửa và lưu tài liệu, sau đó:
git add docs/buffer-cache-notes.md
git commit -m "docs: explain current buffer cache flow"
git push -u origin docs/buffer-cache-notes
```

Mở PR trên GitHub, ghi nội dung đã làm, cách kiểm tra và vấn đề còn tồn tại. Sau khi merge, mọi người cập nhật `main` bằng `git pull --ff-only origin main`.

Không đưa token/mật khẩu hoặc sản phẩm build vào commit. Giữ lại `README`, `LICENSE` và lịch sử xv6 gốc. Quy tắc review là quy ước nhóm; chưa có nghĩa repo đã bật bảo vệ nhánh.

## Kiểm thử và báo cáo

- Kiểm thử có sẵn chạy **bên trong shell xv6**: `usertests`, `stressfs`; lưu kết quả thực tế, không mặc định đã pass.
- Test riêng của đề tài hiện là kế hoạch, xem [tests/README.md](tests/README.md).
- Mỗi lần đo phải ghi commit, phiên bản công cụ, số CPU, dung lượng cache, dữ liệu thử, trạng thái cache ban đầu và cờ read-ahead.
- So sánh cùng điều kiện; phân biệt đọc theo yêu cầu với đọc trước.
- Báo cáo có đủ summary 200–300 từ, hoạt động kỹ thuật, đóng góp từng người, review mã/thuật toán, kiểm thử và TODO.
- Chưa sửa code hoặc chưa đo thì ghi rõ; không dùng số liệu minh họa như kết quả thật.

## Liên kết nhóm

- Trello: **Chưa bổ sung — Hải cập nhật.**
- Figma/FigJam: **Chưa bổ sung — nhóm tạo và cập nhật.**
- Báo cáo tuần 1: [khung báo cáo](reports/week-01/README.md).
- [Nguồn tham khảo](docs/references.md).

## Nguồn gốc và giấy phép

Dự án phát triển từ [MIT xv6-riscv](https://github.com/mit-pdos/xv6-riscv). Commit được kiểm tra khi khởi tạo tài liệu: `f780d7850765d74cbdcd1fff4b822d8829f7c9bd`.

Tham khảo [README gốc](README) và [LICENSE](LICENSE). Tài liệu nhóm phải ghi rõ nội dung tham khảo và phần tự phát triển.

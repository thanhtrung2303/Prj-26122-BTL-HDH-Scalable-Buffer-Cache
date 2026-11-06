# Kết quả triển khai và kiểm thử tuần 3 — Quang

- Người thực hiện: Đồng Xuân Quang — 20233603.
- Ngày kiểm thử: 2026-11-06.
- Nhánh: feat/quang-week03-hash-bucket.
- Commit nền: 125ad9c.
- Commit triển khai cache và test dữ liệu: a4e3fbf.
- Commit bổ sung self-test, mặc định tắt: 8554828.
- QEMU: 1 và 3 CPU, RAM 128 MiB.

## Nội dung triển khai

- 13 hash bucket, mỗi bucket có spinlock riêng.
- Buffer có mapped, hnext, lastuse.
- Cache hit tăng refcnt dưới khóa bucket.
- Cache miss dùng eviction_lock, lấy các khóa bucket theo thứ tự
  tăng dần và tìm lại trước khi cấp phát.
- Chỉ tái sử dụng buffer có refcnt bằng 0.
- bread_ra dùng chung đường cấp phát; hết buffer thì trả 0.
- Có timestamp LRU cơ bản: cập nhật khi brelse/bunpin làm refcnt
  về 0; chọn buffer rảnh có lastuse nhỏ nhất.
- Self-test tùy chọn bằng BCACHE_SELFTEST; mặc định bằng 0.
- Chưa tích hợp kích hoạt read-ahead ở filesystem và thống kê.

## Kết quả

| Kiểm thử | 1 CPU | 3 CPU |
|---|---|---|
| usertests | ALL TESTS PASSED | ALL TESTS PASSED |
| stressfs | Hoàn tất, trở lại shell | Hoàn tất, trở lại shell |
| rwtest | Khớp 4096 byte | Khớp 4096 byte |
| cachecheck | CACHECHECK PASSED | CACHECHECK PASSED |
| Self-test bật | BCACHE SELFTEST PASSED | BCACHE SELFTEST PASSED |
| Self-test tắt, boot và cachecheck | Chưa chạy riêng | PASS |

Các test usertests, stressfs, rwtest và cachecheck ban đầu được
chạy trên mã triển khai trước khi thêm self-test. Mã này được
lưu trong commit a4e3fbf; cachecheck và dòng Makefile được thêm
sau các lần chạy usertests, stressfs, rwtest.

Self-test bật được chạy trên a4e3fbf cộng selftest-source.patch.
Commit 8554828 giữ mã self-test nhưng đặt BCACHE_SELFTEST=0.
Log selftest-off-cpus3.log ghi lần boot và cachecheck ở cấu hình này.

## Phạm vi kiểm thử

cachecheck ghi file 64 KiB, đọc kiểm tra từng byte trong 3 lượt,
kiểm tra EOF, rồi tạo 4 tiến trình, mỗi tiến trình đọc kiểm tra
3 lượt. Chương trình dọn file khi hoàn tất thành công.

Self-test kiểm tra tuần tự:
- Giữ toàn bộ buffer bằng pin.
- Tra cứu lại trả đúng buffer đã có.
- bread_ra trả 0 khi cache đầy và block chưa được ánh xạ.
- Bỏ một pin cho phép tái sử dụng đúng buffer đó.
- Chuyển bucket, bỏ ánh xạ cũ và tìm được ánh xạ mới.
- Các buffer còn pin không bị thay thế.

Self-test chạy trong một tiến trình trước fsinit, kể cả khi
QEMU có 3 CPU; không phải test ép tranh chấp giữa các CPU.

## Minh chứng và điều kiện chạy

Raw log và ảnh nằm trong thư mục này.
Thông tin môi trường ban đầu: environment-cpus1.txt.
Mã triển khai ban đầu: source-changes.patch và cache_stats.h.snapshot.
Phần bổ sung self-test bật: selftest-source.patch.

fs.img được tạo lại khi thêm cachecheck và trong các lượt build
sau thay đổi header. Không coi mọi lần chạy là dùng cùng trạng
thái ảnh đĩa ban đầu.

stressfs không đối chiếu nội dung dữ liệu; chỉ ghi nhận hoàn tất
mà không quan sát thấy panic hoặc treo.

## Giới hạn và bàn giao

- Chưa có test ép hai CPU cùng miss để xác nhận nhánh recheck.
- Chưa chạy riêng nhánh demand panic khi hết buffer.
- Chưa có test chuyên biệt kiểm tra thứ tự timestamp LRU,
  trường hợp bằng timestamp và chọn nạn nhân nhỏ nhất toàn cache.
- Chưa đo contention, throughput hoặc hiệu quả read-ahead.
- Trung review phần timestamp LRU và tích hợp thống kê trên nền này.
- Hải sử dụng bread_ra theo giao diện chung khi tích hợp read-ahead.

# Tuần 4 – kiểm thử của Quang

Nền main: e65098fb84c28d2c26278c6a30bbf033497a9847.
Commit nguồn kiểm thử: ae25e9b04b7cddc7aab16ff2f36832d69558f1f3.
QEMU 10.0.13, GCC riscv64-linux-gnu 14.2.0, RAM 128 MiB, FSSIZE=2000,
NBUF=30, 13 bucket. READ_AHEAD_ENABLED=0 trong tất cả các log.

| Log | CPU | BCACHE_SELFTEST | BCACHE_K01 | Nội dung |
|---|---:|---:|---:|---|
| k01-k03-cpus3.log | 3 | 1 | 1 | K01, K03, K05–K07, cachecheck, rwtest |
| selftest-cpus1.log | 1 | 1 | 0 | K03, K05–K07, cachecheck, rwtest |
| off-cpus1.log | 1 | 0 | 0 | cachecheck, rwtest, usertests |
| off-cpus3.log | 3 | 0 | 0 | cachecheck, rwtest, stressfs, usertests |

K01 dùng khóa thử (ROOTDEV, FSSIZE+300), chỉ gọi bget_source nên không đọc đĩa.
Sau lần tra đầu trượt và sau khi nhả khóa bucket, hart đến trước chờ hart thứ hai
ở cùng điểm. Ngắt tạm tắt trong lúc chờ để tiến trình thứ hai phải chạy trên hart
khác. Sau khi cả hai tới, hai tiến trình đi qua đường cấp phát bình thường. Ca thử
yêu cầu đúng hai lần tới rào, một lần cấp phát, một lần recheck hit, cùng con trỏ
buffer và refcnt cuối bằng 0. Trong bản thử lỗi riêng, bỏ kết quả recheck của
riêng khóa này khiến K01 dừng đúng tại “panic: K01: miss/recheck mismatch”.

K03 chỉ để một buffer rảnh, buộc tái sử dụng lần lượt khác bucket và cùng
bucket. Kiểm tra khóa cũ biến mất, đúng buffer được dùng lại, danh sách không
có vòng, mỗi buffer mapped có mặt đúng một lần ở đúng bucket, và mỗi cặp
(dev, blockno) chỉ có một buffer.

Để chạy lại từ commit nguồn, chỉnh hai cờ trong kernel/param.h theo bảng,
chạy make qemu CPUS=3 (hoặc CPUS=1 theo dòng), nhập các lệnh kiểm thử ở
shell xv6. K01 cần hai hart trở lên, kế hoạch nhóm dùng ba. Cả hai cờ mặc định
bằng 0. Các số đếm trong K01 chỉ đo đường đi của khóa thử. Đối chiếu bộ đếm
thống kê toàn hệ thống và hồi quy C0/C1 cần bản đã tích hợp thống kê, cùng
bread_ra cho block indirect và dữ liệu.

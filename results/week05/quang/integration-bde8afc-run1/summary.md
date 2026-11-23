# Quang — kiểm thử bản tích hợp tuần 5

Source SHA: `bde8afc85922f103066e27c830fb75f0643d63cc`.

Mỗi lượt dùng bản clone riêng và fs.img mới. Chỉ đổi cờ theo patch đi kèm; không sửa kernel trong repo làm việc.

| Cấu hình | CPU | Self-test/K01 | Kết quả | Các kiểm tra đã đạt | Log |
|---|---:|---:|---|---|---|
| C0 | 3 | 1 | PASS | B01, K01, K03, selftest, K07, cachecheck, rwtest | [bde8afc85922-C0-cpus3-selftest1.log](bde8afc85922-C0-cpus3-selftest1.log) |
| C0 | 1 | 0 | PASS | B01, cachecheck, rwtest, usertests | [bde8afc85922-C0-cpus1-selftest0.log](bde8afc85922-C0-cpus1-selftest0.log) |
| C0 | 3 | 0 | PASS | B01, cachecheck, rwtest, stressfs, usertests | [bde8afc85922-C0-cpus3-selftest0.log](bde8afc85922-C0-cpus3-selftest0.log) |
| C1 | 3 | 1 | PASS | B01, K01, K03, selftest, K07, cachecheck, rwtest | [bde8afc85922-C1-cpus3-selftest1.log](bde8afc85922-C1-cpus3-selftest1.log) |
| C1 | 1 | 0 | PASS | B01, cachecheck, rwtest, usertests | [bde8afc85922-C1-cpus1-selftest0.log](bde8afc85922-C1-cpus1-selftest0.log) |
| C1 | 3 | 0 | PASS | B01, cachecheck, rwtest, stressfs, usertests | [bde8afc85922-C1-cpus3-selftest0.log](bde8afc85922-C1-cpus3-selftest0.log) |

K01 xác nhận đường miss/recheck và ánh xạ; chưa xác nhận delta bộ đếm toàn hệ thống của K01. Phần này thuộc đối chiếu của Trung.
Không dùng các lượt này để kết luận D04/D07/D08/R08 hoặc hiệu năng. stressfs chỉ xác nhận chạy xong và trở lại shell, không kiểm tra nội dung dữ liệu.

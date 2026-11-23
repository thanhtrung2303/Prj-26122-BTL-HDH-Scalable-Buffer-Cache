# Quang — kiểm thử bản tích hợp tuần 5

Source SHA: `d09704e0254bf4cb46852c41ee0a5a2d22041df2`.

Mỗi lượt dùng bản clone riêng và fs.img mới. Chỉ đổi cờ theo patch đi kèm; không sửa kernel trong repo làm việc.

| Cấu hình | CPU | Self-test/K01 | Kết quả | Các kiểm tra đã đạt | Log |
|---|---:|---:|---|---|---|
| C0 | 1 | 0 | PASS | B01, cachecheck, rwtest, usertests | [d09704e0254b-C0-cpus1-selftest0.log](d09704e0254b-C0-cpus1-selftest0.log) |
| C0 | 3 | 0 | PASS | B01, cachecheck, rwtest, stressfs, usertests | [d09704e0254b-C0-cpus3-selftest0.log](d09704e0254b-C0-cpus3-selftest0.log) |
| C1 | 1 | 0 | PASS | B01, cachecheck, rwtest, usertests | [d09704e0254b-C1-cpus1-selftest0.log](d09704e0254b-C1-cpus1-selftest0.log) |
| C1 | 3 | 0 | PASS | B01, cachecheck, rwtest, stressfs, usertests | [d09704e0254b-C1-cpus3-selftest0.log](d09704e0254b-C1-cpus3-selftest0.log) |

K01 xác nhận đường miss/recheck và ánh xạ; chưa xác nhận delta bộ đếm toàn hệ thống của K01. Phần này thuộc đối chiếu của Trung.
Không dùng các lượt này để kết luận D04/D07/D08/R08 hoặc hiệu năng. stressfs chỉ xác nhận chạy xong và trở lại shell, không kiểm tra nội dung dữ liệu.

# Quang — kiểm thử bản tích hợp tuần 5

Source SHA: `d09704e0254bf4cb46852c41ee0a5a2d22041df2`.

Mỗi lượt dùng bản clone riêng và fs.img mới. Chỉ đổi cờ theo patch đi kèm; không sửa kernel trong repo làm việc.

| Cấu hình | CPU | Self-test/K01 | Kết quả | Các kiểm tra đã đạt | Log |
|---|---:|---:|---|---|---|
| C0 | 3 | 1 | PASS | B01, K01, K03, selftest, K07, cachecheck, rwtest | [d09704e0254b-C0-cpus3-selftest1.log](d09704e0254b-C0-cpus3-selftest1.log) |
| C1 | 3 | 1 | PASS | B01, K01, K03, selftest, K07, cachecheck, rwtest | [d09704e0254b-C1-cpus3-selftest1.log](d09704e0254b-C1-cpus3-selftest1.log) |

K01 xác nhận đường miss/recheck và ánh xạ; chưa xác nhận delta bộ đếm toàn hệ thống của K01. Phần này thuộc đối chiếu của Trung.
Không dùng các lượt này để kết luận D04/D07/D08/R08 hoặc hiệu năng. stressfs chỉ xác nhận chạy xong và trở lại shell, không kiểm tra nội dung dữ liệu.

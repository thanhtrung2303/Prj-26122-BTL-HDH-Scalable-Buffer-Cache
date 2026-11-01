# Kiểm thử buffer cache

Kế hoạch chi tiết hiện hành: [test_plan.md](test_plan.md), gồm ca dữ liệu, hash bucket/LRU, đọc trước, thống kê và đo hiệu năng. Các lệnh test mới trong kế hoạch là giao diện cần triển khai; không coi là đã có mã hoặc PASS.

## Phân công và mốc trong kế hoạch 6 tuần

| Người chính | Phần kiểm thử | Mốc |
|---|---|---|
| Quang | Baseline usertests/stressfs; hash bucket, chống trùng, hết buffer; regression và contention | Baseline tuần 2; cache tuần 3; regression/đo tuần 5 |
| Trung | LRU, pin/unpin, bộ đếm, snapshot/reset và tổng hợp số liệu | LRU tuần 3; thống kê tuần 4; kiểm tra/đo tuần 5 |
| Hải | Test ghi–đọc, repeat/capacity/concurrent; read-ahead/EOF/indirect/shared file; ON/OFF | Test đầu tiên + log B0 tuần 2; test cache tuần 3; read-ahead tuần 4–5 |
| Cả nhóm | Build/boot trên máy từng người, kiểm tra bản bàn giao và tập demo | Tuần 2 và regression theo thay đổi; demo tuần 6 |

Các ID và cổng ưu tiên theo [project-plan.md](../docs/project-plan.md#6-kiểm-thử-ưu-tiên-và-mốc-nghiệm-thu). 47 ca có thể dùng chung chương trình/hook; không cần viết 47 chương trình riêng. Không bỏ test correctness chỉ để kịp hạn; ghi rõ ca chưa chạy/chưa đạt.

Chương trình C chạy trong xv6 đặt ở user/ và đăng ký UPROGS. Script host đặt ở scripts/. Không mặc định có bcachetest hoặc test hook khi chưa kiểm tra mã. Mỗi log ghi SHA, môi trường, CPU, cache/config ON/OFF, dữ liệu và kết quả; log thất bại cũng được lưu.

# Kế hoạch dự án 6 tuần — Scalable Buffer Cache

> Tổng thời gian: **6 tuần**, hiện tại là **tuần 2**. Hoàn tất tích hợp chức năng cuối tuần 4, kiểm thử/đo đạc tuần 5, bàn giao tuần 6. Mốc dưới đây là hạn thực hiện, không phải xác nhận công việc đã hoàn thành.

## 1. Phạm vi và nguyên tắc

- Chức năng: hash bucket có khóa riêng, timestamp LRU bằng lru_clock, thống kê demand/read-ahead và đọc trước tuần tự đồng bộ tối đa 2 block.
- Read-ahead bật/tắt bằng cấu hình biên dịch; không thêm cơ chế bất đồng bộ, flusher hoặc cache resize trong phạm vi hiện tại. Nếu giảng viên yêu cầu khác, điều chỉnh phạm vi ngay tuần 2.
- Còn tuần 2 để chốt thiết kế và bốn tuần tiếp theo để triển khai/bàn giao. Không giữ công việc bắt buộc đến tuần 7–9.
- Mỗi việc có một người chính, người đọc chéo, hạn và minh chứng. Người chính tự sửa lỗi phần mình; Trung tích hợp bản chung.
- Báo cáo viết dần từ tuần 3. Đầu tuần 6 chốt chức năng, chỉ sửa lỗi và hoàn thiện bàn giao.

## 2. Trách nhiệm xuyên suốt

| Thành viên | Trách nhiệm chính | Người đọc chéo |
|---|---|---|
| Đồng Xuân Quang — 20233603 | Cấu trúc hash cache, khóa, cấp phát/chuyển buffer, chống trùng, hết buffer; baseline và contention | Trung |
| Nguyễn Thành Trung — 20233680 | Timestamp LRU, pin/unpin, thống kê, snapshot/reset, tích hợp; tổng hợp báo cáo/slides | Quang kiểm tra LRU/khóa; Hải kiểm tra thống kê |
| Nguyễn Xuân Hải — 20233383 | Sequential read-ahead, trạng thái file, ánh xạ không cấp phát, ON/OFF; test dữ liệu và demo | Trung; Quang kiểm tra tương tác cache |

Hải cập nhật board/Trello nếu nhóm dùng; mỗi người cập nhật thẻ của mình. Không mặc định đã có board hoặc thao tác ngoài GitHub đã được thực hiện.

## 3. Mốc công việc từng tuần

| Tuần | Quang | Trung | Hải | Đầu ra chung |
|---|---|---|---|---|
| 1 — lịch sử | Môi trường, tổ chức cache gốc | Repo, luồng xử lý và báo cáo | Yêu cầu và kế hoạch kiểm thử ban đầu | Giữ minh chứng và phân công cũ; không tự đánh dấu hoàn thành |
| 2 — hiện tại | Chốt hash-bucket.md theo giao diện chung; hoàn thiện baseline usertests/stressfs | Chốt LRU, thống kê, integration và test plan; tổng hợp tuần 2 | Hoàn thiện thiết kế read-ahead, test ghi–đọc so sánh nội dung + UPROGS + log baseline | Thiết kế nhất quán, baseline có minh chứng, test đầu tiên chạy được |
| 3 | Triển khai hash bucket, recheck miss, chuyển buffer, cấp phát tùy chọn | Triển khai lru_clock, chọn nạn nhân, brelse/bunpin; tích hợp với Quang | Test repeat/capacity/concurrent; trạng thái tuần tự và ánh xạ không cấp phát | Hash bucket + LRU chạy đúng khi read-ahead OFF |
| 4 | Kiểm tra tranh chấp, chống trùng, hết buffer; sửa cấp phát | Hoàn thiện nguồn yêu cầu, bộ đếm, syscall, cache_stat và tích hợp | Hoàn thiện đọc trước 2 block, ON/OFF, EOF/indirect, shared file | Bản đủ chức năng cuối tuần; có dữ liệu đo thống kê |
| 5 | Regression, khóa, so sánh baseline/cache mới | Kiểm tra LRU/thống kê, tổng hợp số liệu, sửa tích hợp và nháp báo cáo | Kiểm thử dữ liệu/read-ahead, ON/OFF, tình huống biên và sửa lỗi | Bản ổn định, log đúng SHA, bảng số liệu và báo cáo nháp |
| 6 | Viết/hoàn thiện phần hash bucket và đồng bộ; giải thích khóa khi bảo vệ | Chốt báo cáo/slides, bản bàn giao, kịch bản demo và tổng hợp đóng góp | Hoàn thiện phần read-ahead/test; chuẩn bị dữ liệu và chạy demo | Chốt chức năng đầu tuần; sửa lỗi cuối, tập bảo vệ, bàn giao trước cuối tuần |

## 4. Việc phải bàn giao trong tuần 2

| Người chính | Công việc | Đầu ra và điều kiện đạt | Người đọc chéo |
|---|---|---|---|
| Quang | Chốt thiết kế hash bucket | Cấu trúc/head/hnext, thứ tự khóa, LRU, nguồn yêu cầu, hit khi recheck, demand panic/RA return 0; nhất quán integration | Trung |
| Quang | Baseline | SHA, môi trường, số CPU, log usertests và stressfs; ghi đúng kết quả/lỗi | Hải |
| Trung | Thiết kế LRU và thống kê | Hai đặc tả có khóa, vị trí cập nhật, giao diện và ví dụ; không còn giao diện mâu thuẫn | Quang/Hải theo chuyên môn |
| Trung | Tổng hợp thiết kế/báo cáo | integration.md, kế hoạch 6 tuần, sơ đồ hoặc luồng xử lý, link minh chứng và TODO có chủ sở hữu | Cả nhóm |
| Hải | Chốt read-ahead | Phát hiện tuần tự, nơi lưu/khóa trạng thái, readi_ra/bread_ra, ánh xạ, I/O đồng bộ, ON/OFF, thiếu buffer/EOF | Trung |
| Hải | Test ghi–đọc đầu tiên | Source trong user/, đăng ký UPROGS, kiểm tra từng byte và số byte, log chạy trên B0 | Quang |
| Cả nhóm | Review thiết kế và môi trường | Góp ý được xử lý/ghi TODO; build/boot trên máy từng người có log | Trung tổng hợp |

Mã thử nghiệm sẵn có trong PR của Hải phải đối chiếu hợp đồng trước khi tích hợp. Không dùng số dòng code hoặc mô tả PR thay cho test/log.

## 5. Phối hợp sửa file và bàn giao giữa tuần

- Quang chủ trì cấu trúc/đường cấp phát trong bio.c và buf.h; bàn giao giao diện cache chung trước để Trung tích hợp LRU/thống kê.
- Trung triển khai LRU/thống kê trên cùng nền hash cache; không giữ một phiên bản cấp phát khác.
- Hải chủ trì file.h/file.c/fs.c và test user; gọi API chung, không viết lại hash cache.
- defs.h/param.h/Makefile: người thêm chức năng chịu trách nhiệm phần khai báo/đăng ký; Trung kiểm tra trước khi tích hợp.
- Đầu tuần xác định thẻ và điều kiện đạt; giữa tuần bàn giao PR chạy được cho phần phụ thuộc; cuối tuần đọc chéo, lưu log và tổng hợp báo cáo. Hạn cụ thể trên lịch được nhóm điền theo lịch môn học, không tự suy ra ngày từ số tuần.
- Mỗi thay đổi có PR nhỏ, hướng dẫn kiểm tra và link log; không chờ tới cuối tuần 4 mới ghép ba phần lần đầu.

## 6. Kiểm thử ưu tiên và mốc nghiệm thu

Danh mục chi tiết: [test_plan.md](../tests/test_plan.md). 47 ca là danh mục kiểm tra; nhiều ca dùng chung một chương trình/hook, không phải 47 chương trình độc lập.

| Mốc | Ca ưu tiên | Điều kiện |
|---|---|---|
| Tuần 2 | B01–B04 trên B0 theo phạm vi từng người | Baseline và test ghi–đọc có log; không yêu cầu mọi test sau tích hợp chạy ngay |
| Tuần 3 | B01–B04, D01–D03, K01–K07, K09 trên C0 | Dữ liệu đúng, không trùng buffer, không thu hồi buffer đang giữ/pin; LRU đúng |
| Tuần 4 | D04–D10, R01–R08, S01–S11; lặp regression C0/C1 | Đọc trước/thống kê tích hợp, ON/OFF đúng, không lỗi EOF/indirect hoặc refcnt |
| Tuần 5 | Chạy toàn bộ nhóm chức năng B/D/K/R/S, gồm K08/K10; P01–P04 | Có log; lỗi phải sửa hoặc ghi chưa đạt; đo contention trước khi kết luận giảm contention |
| Tuần 6 | Chạy lại regression/ca bị ảnh hưởng bởi sửa lỗi và rehearsal demo | Bản bàn giao có thể tái hiện; không tự ghi PASS cho ca chưa chạy |

Không giảm tiêu chí đúng dữ liệu/deadlock để kịp hạn. Nếu chậm: dừng mở rộng, dùng chung test/hook và ưu tiên sửa lỗi; báo rõ mục tiêu nào chưa đạt. Hiệu năng không có ngưỡng phần trăm tự đặt sau khi đo; báo cả khi RA đồng bộ không tăng tốc.

## 7. Theo dõi công việc và minh chứng

Board dùng các trạng thái: Backlog → Tuần này → Đang làm → Chờ kiểm tra → Hoàn thành. Mỗi thẻ có người chính, người đọc chéo, hạn cuối tuần tương ứng, checklist, điều kiện đạt và link PR/log.

Chỉ hoàn thành khi người đọc chéo xác nhận và có minh chứng. Thất bại/treo phải lưu log và việc sửa; không xóa log lỗi để thay bằng kết quả PASS. Báo cáo mỗi tuần ghi đóng góp, phần chưa xong và việc tuần sau có chủ sở hữu.

Phân công tuần 1 được lưu tại [week-01-tasks.md](week-01-tasks.md). Các tài liệu khảo sát tuần 1 là nguồn lịch sử; kế hoạch hiện hành là file này và [integration.md](design/integration.md).

# Kế hoạch kiểm thử và demo — Project 26122

Bàn giao `kernel/bio.c` mới, test contention, throughput và read-ahead. Demo gồm so sánh tranh chấp khóa trước/sau và tốc độ đọc tuần tự khi tắt/bật read-ahead. Write-back flusher và cấu hình dung lượng cache lúc boot là mục mở rộng.

## Cấu hình chạy

| Mã | Cấu hình |
|---|---|
| B0 | xv6 nền, trước khi thay đổi buffer cache. Ghi SHA thực tế. |
| C0 | Cache mới với hash bucket và timestamp LRU, `READ_AHEAD_ENABLED=0`. |
| C1 | Cùng mã cache với C0, `READ_AHEAD_ENABLED=1`, cửa sổ đọc trước 2 block. |

Giữ dung lượng cache, dữ liệu đầu vào, ảnh đĩa ban đầu và phiên bản công cụ tương đương trong từng phép so sánh. Hồi quy C0 trên 1 và 3 CPU. Chạy benchmark chính với 3 CPU. Tắt `BCACHE_SELFTEST` cho hồi quy thông thường và benchmark. Bật riêng cho các ca kernel rồi ghi cấu hình trong log.

## 1. Hồi quy và tính đúng của buffer cache

| ID | Cách chạy và tiêu chí đạt | Minh chứng hiện có |
|---|---|---|
| B01 | Build sạch và boot đến shell trên 1 và 3 CPU. | Có log tuần 3. |
| B02 | Chạy `usertests` đến `ALL TESTS PASSED`. | Có log tuần 3. |
| B03 | Chạy `stressfs`; quay về shell, không panic hoặc treo. | Có log tuần 3. |
| B04 | Dùng `rwtest` ghi, đóng/mở lại rồi kiểm tra đủ 4.096 byte. | Có log tuần 3. |
| D01 | Dùng `cachecheck` đọc cùng file nhiều lần và đối chiếu từng byte. | Có log tuần 3. |
| D02 | Dùng `cachecheck` đọc đúng file 64 KiB khi file lớn hơn cache. | Có log tuần 3. |
| D03 | Nhiều tiến trình con đọc file chung và file riêng trong 20 vòng. Dữ liệu đúng và mọi tiến trình kết thúc thành công. | Đạt một phần. `cachecheck` có 4 tiến trình đọc file chung, mỗi tiến trình 3 lượt; `rwtest` có cha/con đọc file chung. |
| K01 | Dùng barrier ép hai CPU cùng miss một khóa trước đường recheck. Xác nhận chỉ có một ánh xạ và hai bên nhận đúng buffer. | Cần bổ sung. Ca tra cứu lặp hiện có chạy tuần tự. |
| K03 | Buộc tái sử dụng buffer trong cùng bucket và chuyển sang bucket khác. Xác nhận ánh xạ cũ bị gỡ, ánh xạ mới tra được và danh sách không mất hoặc lặp buffer. | Đạt một phần. Self-test đã kiểm tra chuyển bucket. |
| K05 | Giữa hai buffer rảnh ở hai bucket, chọn buffer có timestamp cũ hơn. | PASS trong lượt self-test 3 CPU. |
| K06 | Giữ nguyên timestamp khi refcnt giảm 2→1, cập nhật khi về 0 và không thay thế buffer đang pin. | PASS trong lượt self-test 3 CPU. |
| K07 | Tiến trình chờ sleeplock giữ tham chiếu; chỉ cập nhật timestamp sau lần nhả cuối. | PASS trong lượt self-test 3 CPU. |
| K09 | Giữ hết buffer trong harness kernel; `bread_ra()` cho block chưa ánh xạ trả 0. Nhả hết tham chiếu, không panic hoặc rò khóa; kiểm tra một skip khi dùng bộ đếm. | Đã có log trả 0 khi cache đầy. Kiểm tra bộ đếm còn lại. |

Tái sử dụng `rwtest`, `cachecheck` và self-test hiện có. Giữ bài đọc lặp, file lớn và đọc cha/con trong `rwtest`; dùng `cachecheck` làm minh chứng chính cho D01–D03 để tránh tổng hợp hai lần. Giữ các kiểm tra pin, tra cứu lặp và chuyển bucket của Quang trong self-test.

Minh chứng tuần 3 thuộc nhiều phiên bản mã. Chạy lại B01–B04 và D01–D03 trên cùng SHA của bản tích hợp. Lưu lượt self-test K05–K07 riêng với lượt hồi quy thường. Xem [Quang](../results/week03/quang/summary.md), [Trung](../results/week03/trung/summary.md) và [Hải](https://github.com/thanhtrung2303/Prj-26122-BTL-HDH-Scalable-Buffer-Cache/pull/11).

## 2. Read-ahead

| ID | Cách chạy và tiêu chí đạt | Minh chứng hiện có |
|---|---|---|
| D04 | Đọc tuần tự theo block cùng một file trên C0/C1; đối chiếu từng byte và tổng số byte. | Có bài đọc dữ liệu; cần cặp log ON/OFF trên bản tích hợp. |
| D07 | Kiểm tra file rỗng, BSIZE−1, BSIZE và BSIZE+1; đọc thêm sau EOF. C0/C1 trả đúng số byte, C1 không nạp block ngoài file hoặc cấp phát block mới. | Đạt một phần. `cachecheck` đã kiểm tra EOF cho file 64 KiB. |
| D08 | Đọc tuần tự qua ranh giới direct/indirect trên C0/C1. Dữ liệu đúng và đường read-ahead chỉ tra ánh xạ hiện có. | Đã có bài file lớn; cần minh chứng qua indirect với read-ahead bật. |
| R08 | Chạy cùng workload tuần tự trên C0/C1. Trace C0 không có READ_AHEAD; C1 có kích hoạt và xét tối đa 2 block dữ liệu mỗi lần. | PR #11 có ghi nhận debug kích hoạt; còn thiếu cặp trace đối chiếu. |
| S02 | Trong fixture cô lập, để read-ahead nạp block chưa có, sau đó demand đọc block đó. Trace hoặc bộ đếm xác nhận I/O thuộc read-ahead và demand hit. | Cần bổ sung. |

Dùng chung workload D04, R08 và P02 khi phù hợp. Thu trace hoặc kiểm tra kernel để xác nhận việc nạp trước và tra ánh xạ. Đo tốc độ ở lượt riêng nếu trace làm sai lệch thời gian.

## 3. Benchmark và demo

| ID | So sánh | Workload và chỉ số |
|---|---|---|
| P04 — Demo contention | B0 và C0 | Cùng workload đồng thời và cùng số CPU. Gắn bộ đo khóa theo cùng phương pháp trên hai bản. Báo tổng lần thử lấy khóa thất bại và số lần acquire. C0 báo riêng bucket, eviction và `lru_clock`. |
| P01 — Throughput | B0 và C0 | Có thể dùng workload P04. Giữ số tiến trình, vòng lặp và lượng dữ liệu cố định. Báo thời gian và MiB/s hoặc thao tác/giây, kiểm tra dữ liệu đúng. Ghi rõ trạng thái instrumentation và ưu tiên lượt đo thời gian riêng. |
| P02 — Demo tốc độ đọc tuần tự | C0 và C1 | Đọc cùng file vượt cache, cùng số byte và cùng trạng thái cache ban đầu. Báo thời gian và MiB/s; ghi lượt đọc đĩa demand, read-ahead và tổng khi bộ đếm sẵn sàng. |

Chạy tối thiểu 5 lượt mỗi cấu hình, lưu từng mẫu và dùng trung vị để so sánh. Nếu dùng tick, ghi đơn vị và xác nhận chu kỳ trước khi quy đổi. Với cold-cache, khởi động lại với dữ liệu chuẩn bị sẵn theo cùng quy trình. Reset bộ đếm chỉ đặt lại thống kê. Công bố số đo thực tế kể cả khi read-ahead không tăng tốc.

Quang phụ trách P04 và workload P01. Hải phụ trách D04/D07/D08/R08 và P02. Trung phụ trách K05–K07, S02 và tổng hợp bảng so sánh. Quang phụ trách K01/K03/K09 và phối hợp chạy hồi quy.

## 4. Nghiệm thu và lưu minh chứng

- Lưu log gốc dưới `results/integration/`. Mỗi lượt ghi SHA, diff nếu có, B0/C0/C1, số CPU, cờ self-test/read-ahead, dữ liệu, trạng thái cache, lệnh và kết quả.
- Bảng kết quả ghi ID, PASS/FAIL/TIMEOUT/CHƯA CHẠY hoặc ĐẠT MỘT PHẦN, và link log. Lưu cả lượt thất bại.
- Bản nộp có mã `bio.c` mới, log hồi quy và bảng P01/P02/P04. Demo dùng P04 trước, sau đó P02.

# Kế hoạch dự án 9 tuần — Scalable Buffer Cache

> **Trạng thái:** Kế hoạch dự kiến. Nhóm cần xác nhận ngày bắt đầu, hạn cụ thể và phân công cuối cùng. Không xem đầu việc là hoàn thành nếu chưa có minh chứng.

## 1. Mục tiêu và nguyên tắc

- Hoàn thành các chức năng bắt buộc **trước hoặc vào cuối tuần 6**.
- Mỗi đầu việc phải có người phụ trách, hạn, checklist, tiêu chí hoàn thành và liên kết minh chứng.
- Chỉ chuyển thẻ sang **Hoàn thành** khi tiêu chí đạt và có link tới commit, PR, kết quả kiểm thử hoặc tài liệu.
- Những tính năng mở rộng không bắt buộc được giữ trong **Backlog** cho đến khi nhóm xác nhận phạm vi.

## 2. Các cột Trello

1. **Backlog** — việc chưa được chọn làm trong tuần hiện tại, gồm tính năng tương lai và cải tiến tùy chọn.
2. **Tuần này** — việc nhóm cam kết thực hiện trong tuần.
3. **Đang làm** — việc đã bắt đầu; thẻ phải có người phụ trách.
4. **Chờ kiểm tra** — đã có đầu ra, đang chờ người khác đọc chéo hoặc chạy kiểm thử.
5. **Hoàn thành** — đã đạt điều kiện hoàn thành và gắn minh chứng.

Mỗi thẻ cần có:
- **Người phụ trách:** một người chịu trách nhiệm chính; người hỗ trợ ghi trong mô tả.
- **Hạn hoàn thành:** ngày cụ thể do nhóm thống nhất; nếu chưa chốt, ghi “chưa chốt” và xác nhận ở buổi họp.
- **Checklist:** các bước nhỏ có thể kiểm tra được.
- **Điều kiện hoàn thành:** kết quả quan sát/kiểm chứng được, không chỉ là “đã làm”.
- **Minh chứng:** link tài liệu, commit, PR, lệnh kiểm thử hoặc log kết quả.

## 3. Kế hoạch 9 tuần

| Tuần | Mục tiêu và công việc | Người phụ trách dự kiến | Đầu ra / điều kiện đạt |
|---|---|---|---|
| 1 | Xác nhận môi trường và repo; khảo sát yêu cầu; tạo Trello; lập kế hoạch | Quang: môi trường; Trung và Hải: khảo sát, yêu cầu; cả nhóm: Trello | Hướng dẫn môi trường; tài liệu yêu cầu; board có cột, thẻ và chủ sở hữu; kế hoạch được nhóm xem xét |
| 2 | Chốt thiết kế, quy tắc khóa và ma trận kiểm thử; đọc chéo tài liệu | Cả nhóm; Trung tổng hợp báo cáo | Thiết kế được review; rủi ro đồng bộ và các ca kiểm thử được ghi lại |
| 3 | Triển khai/phối hợp hash bucket và khóa theo bucket | Quang phụ trách chính; nhóm review | Build được; test cơ bản chạy; không tạo bộ đệm trùng trong các trường hợp đã kiểm tra |
| 4 | Triển khai/kiểm tra timestamp LRU và xử lý truy cập đồng thời | Trung phụ trách chính; Quang hỗ trợ đồng bộ | Quy tắc chọn nạn nhân được kiểm tra; kiểm thử truy cập đồng thời được chạy và ghi kết quả |
| 5 | Bộ đếm hit/miss, thu thập số liệu nền và hoàn thiện chương trình kiểm thử | Trung: thống kê/tích hợp; Hải: test và tổng hợp số liệu | Định nghĩa chỉ số rõ ràng; có lệnh chạy và log/số liệu thực tế, hoặc ghi rõ vấn đề còn tồn tại |
| 6 | Hoàn thiện Read-ahead và tích hợp các chức năng bắt buộc | Hải phụ trách Read-ahead/test theo phân công dự kiến; cả nhóm tích hợp | **Mốc chức năng bắt buộc:** build/boot được; các test đúng đắn và tích hợp đã chạy; Read-ahead được kiểm tra; lỗi còn lại được ghi rõ |
| 7 | Kiểm thử tổng thể, đo và so sánh hiệu năng, sửa lỗi ưu tiên | Cả nhóm; Hải tổng hợp số liệu | Có kết quả đo thực tế, mô tả môi trường và phân tích; không dùng số liệu ước lượng thay số liệu đo |
| 8 | Hoàn thiện báo cáo, slide, kịch bản demo và ổn định mã nguồn | Trung tổng hợp báo cáo; từng thành viên viết phần mình | Bản báo cáo/slide được đọc chéo; demo có các bước tái hiện |
| 9 | Dự phòng, sửa lỗi cuối, luyện bảo vệ và bàn giao | Cả nhóm | Có thể tái hiện build/test/demo; tài liệu, link và đóng góp được xác nhận |

**Lưu ý:** Phân công từ tuần 3 trở đi là phương án dự kiến theo kế hoạch hiện có, cần nhóm xác nhận trước khi coi là phân công chính thức. Nếu chức năng bắt buộc còn lỗi ở tuần 6, ưu tiên sửa lỗi và hoãn tính năng mở rộng.

## 4. Các thẻ cần tạo/cập nhật trên Trello

Ngày cụ thể chưa được cung cấp; vì vậy dùng hạn tương đối **cuối tuần tương ứng** cho đến khi nhóm thống nhất ngày bắt đầu. Hãy thay bằng ngày thực tế trên Trello.

### Thẻ A — Xác nhận môi trường và hướng dẫn chạy
- **Danh sách:** Tuần này (tuần 1)
- **Phụ trách:** Quang
- **Hạn:** Cuối tuần 1 — ngày cụ thể chưa chốt
- **Checklist:** [ ] ghi phiên bản/revision; [ ] ghi công cụ và phụ thuộc; [ ] ghi lệnh build/run; [ ] xác minh thành viên khác có thể làm theo.
- **Điều kiện hoàn thành:** hướng dẫn chạy được kiểm chứng trên môi trường nhóm và có link tài liệu/log.
- **Minh chứng:** link hướng dẫn môi trường hoặc commit tài liệu.

### Thẻ B — Khảo sát yêu cầu và luồng xử lý cache
- **Danh sách:** Tuần này (tuần 1)
- **Phụ trách:** Trung và Hải
- **Hạn:** Cuối tuần 1 — ngày cụ thể chưa chốt
- **Checklist:** [ ] tóm tắt yêu cầu bắt buộc; [ ] mô tả luồng lấy/nhả buffer; [ ] xác định rủi ro đồng bộ; [ ] ghi câu hỏi chưa rõ; [ ] đọc chéo.
- **Điều kiện hoàn thành:** tài liệu được Trung và Hải đọc chéo, các điểm chưa rõ được ghi lại.
- **Minh chứng:** [Tài liệu phân tích của Hải](NguyenXuanHai202233383/buffer-cache-analysis.md), [kế hoạch kiểm thử](NguyenXuanHai202233383/testing-plan.md).

### Thẻ C — Tạo board Trello và kế hoạch 9 tuần
- **Danh sách:** Tuần này (tuần 1)
- **Phụ trách:** Hải (đầu mối dự kiến); cả nhóm xác nhận
- **Hạn:** Cuối tuần 1 — ngày cụ thể chưa chốt
- **Checklist:** [ ] tạo 5 danh sách; [ ] tạo thẻ tuần 1; [ ] thêm người phụ trách/hạn/checklist/tiêu chí; [ ] đưa việc tương lai vào Backlog; [ ] gắn link tài liệu và PR; [ ] chia sẻ board cho nhóm.
- **Điều kiện hoàn thành:** thành viên có quyền xem board và nhìn từng thẻ biết ai làm gì, hạn nào, đầu ra ở đâu.
- **Minh chứng:** board Trello của nhóm và file kế hoạch này.

### Thẻ D — Đọc chéo tài liệu luồng xử lý của Trung
- **Danh sách:** Tuần này (tuần 1)
- **Phụ trách:** Hải
- **Hạn:** Thứ Bảy tuần 1 — ngày cụ thể chưa chốt
- **Checklist:** [ ] đọc tài liệu luồng xử lý; [ ] đối chiếu với yêu cầu; [ ] ghi chỗ thiếu/mâu thuẫn; [ ] gửi góp ý cho Trung; [ ] xác nhận đã xử lý hoặc ghi TODO.
- **Điều kiện hoàn thành:** có góp ý cụ thể và trạng thái xử lý; không chỉ ghi “đã đọc”.
- **Minh chứng:** link tài liệu/PR của Trung và comment hoặc ghi chú review.

### Thẻ E — Đọc chéo checklist yêu cầu và kế hoạch của Hải
- **Danh sách:** Tuần này (tuần 1)
- **Phụ trách:** Quang
- **Hạn:** Thứ Bảy tuần 1 — ngày cụ thể chưa chốt
- **Checklist:** [ ] kiểm tra checklist yêu cầu; [ ] kiểm tra tính khả thi của kế hoạch; [ ] đánh dấu điểm thiếu; [ ] gửi góp ý; [ ] xác nhận TODO.
- **Điều kiện hoàn thành:** checklist và kế hoạch có phản hồi review được ghi nhận.
- **Minh chứng:** [Kế hoạch kiểm thử](NguyenXuanHai202233383/testing-plan.md), [kế hoạch 9 tuần](project-plan.md).

### Thẻ F — Bàn giao bản nháp và tổng hợp báo cáo tuần
- **Danh sách:** Tuần này (tuần 1)
- **Phụ trách:** Trung tổng hợp; Quang và Hải bàn giao phần mình
- **Hạn:** Thứ Bảy tuần 1 — ngày cụ thể chưa chốt
- **Checklist:** [ ] từng người cập nhật trạng thái; [ ] gắn link đầu ra; [ ] ghi vấn đề/TODO; [ ] đọc chéo; [ ] Trung tổng hợp báo cáo.
- **Điều kiện hoàn thành:** báo cáo phản ánh đúng công việc đã làm, phần chưa làm và liên kết minh chứng.
- **Minh chứng:** báo cáo nhóm, các PR và board Trello.

### Thẻ G — Chốt tiến độ và kiểm tra quyền truy cập
- **Danh sách:** Tuần này (tuần 1)
- **Phụ trách:** Cả nhóm
- **Hạn:** Trước hạn nộp tuần 1 — ngày cụ thể chưa chốt
- **Checklist:** [ ] họp ngắn; [ ] kiểm tra link mở được; [ ] kiểm tra quyền xem; [ ] xác nhận đóng góp; [ ] chốt TODO và mốc tuần 2.
- **Điều kiện hoàn thành:** mọi đầu ra có thể truy cập bởi nhóm/giảng viên theo yêu cầu; TODO có người phụ trách.
- **Minh chứng:** link board, repo, PR và báo cáo.

## 5. Việc chung và lịch bàn giao tuần 1

| Thời điểm | Việc cần làm |
|---|---|
| Thứ Năm–Thứ Sáu | Quang xác nhận môi trường; Trung và Hải viết tài liệu khảo sát/yêu cầu; tạo và cập nhật Trello |
| Thứ Bảy | Bàn giao bản nháp, đọc chéo, sửa điểm thiếu; Trung tổng hợp báo cáo |
| Trước hạn nộp | Họp ngắn chốt tiến độ; kiểm tra link/quyền xem; xác nhận đóng góp và TODO |

### Phân công đọc chéo
- **Trung** kiểm tra hướng dẫn chạy và ghi chú tổ chức cache của Quang.
- **Hải** kiểm tra tài liệu luồng xử lý của Trung.
- **Quang** kiểm tra checklist yêu cầu và kế hoạch của Hải.

## 6. Quy tắc chuyển trạng thái trên Trello

- **Backlog → Tuần này:** nhóm thống nhất ưu tiên và có người phụ trách.
- **Tuần này → Đang làm:** người phụ trách đã bắt đầu và cập nhật checklist.
- **Đang làm → Chờ kiểm tra:** có đầu ra xem được và hướng dẫn kiểm tra.
- **Chờ kiểm tra → Hoàn thành:** người review xác nhận đạt tiêu chí và minh chứng hoạt động.
- Nếu không đạt, chuyển lại **Đang làm** và ghi rõ việc cần sửa.

## 7. Liên kết đầu ra của Hải

- [README đóng góp cá nhân](NguyenXuanHai202233383/README.md)
- [Phân tích thiết kế Buffer Cache](NguyenXuanHai202233383/buffer-cache-analysis.md)
- [Kế hoạch kiểm thử](NguyenXuanHai202233383/testing-plan.md)
- [Pull Request #3](https://github.com/thanhtrung2303/Prj-26122-BTL-HDH-Scalable-Buffer-Cache/pull/3)

Các tài liệu trên là đầu ra tài liệu. Không xem chúng là bằng chứng cho việc triển khai code hoặc kiểm thử thành công nếu chưa có minh chứng tương ứng.

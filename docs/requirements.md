# Yêu cầu và nghiệm thu dự kiến

**Phụ trách: Hải. Đây là phân tích yêu cầu, chưa phải kết quả triển khai.**

| Mã | Yêu cầu | Sản phẩm dự kiến | Kiểm chứng sơ bộ |
|---|---|---|---|
| R01 | Hash bucket, khóa từng bucket | Cache chia bucket | Review code; kiểm tra truy cập đồng thời, không trùng buffer cho cùng dev/block |
| R02 | Timestamp LRU | Chọn buffer đủ điều kiện có mốc sử dụng cũ nhất theo thiết kế đã chốt | Tình huống thay thế buffer; kiểm tra refcnt và pin |
| R03 | Sequential read-ahead | Cơ chế đọc trước, có đối chứng bật/tắt | Tính đúng dữ liệu; đọc tuần tự bật/tắt cùng điều kiện |
| R04 | Hit/miss statistics | Bộ đếm và cách xem | Tình huống có kiểm soát; chốt rõ định nghĩa trước khi đo |
| R05 | Contention/throughput/read-ahead tests | Test, log và tổng hợp | Chạy lại được với cấu hình ghi rõ |
| R06 | Demo cuối kỳ | Kịch bản so sánh | Contention trước/sau; tốc độ tuần tự bật/tắt |

## Nâng cao
- Write-back flusher.
- Cấu hình dung lượng cache khi boot.
Chỉ làm khi phần bắt buộc ổn định.

## Giải thích khái niệm — Hải bổ sung
Với hash bucket, LRU, read-ahead, hit/miss: nêu ý nghĩa, vấn đề giải quyết, ví dụ và nguồn đã đọc.

## Câu hỏi cần chốt ở tuần 2
- Số bucket và quy tắc ánh xạ?
- Thứ tự lấy khóa và cách cấp phát lại buffer?
- Thời điểm cập nhật timestamp?
- Phân biệt hit/miss theo yêu cầu với read-ahead thế nào?
- Kích hoạt read-ahead ở đâu, bao nhiêu block?
- Chỉ số contention và phương pháp đo?
- Các yêu cầu bàn giao chung trong Overview của môn học?

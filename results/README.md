# Kết quả và log

Chỉ lưu số liệu thực tế; hiện chưa có kết quả benchmark của nhóm.

Gợi ý tổ chức theo lần đo: results/<ngay>-<ten-test>/ với log và ghi chú.
Ghi: commit, công cụ, CPU/RAM QEMU, số buffer/bucket, dữ liệu thử, cold/warm cache, read-ahead bật/tắt, số lần lặp, lệnh chạy, kết quả từng lần.

Giữ log gốc, không chỉ lưu ảnh. Phân biệt đọc theo yêu cầu và đọc trước; giải thích các lần lỗi. Không commit ảnh đĩa fs.img hoặc sản phẩm build.

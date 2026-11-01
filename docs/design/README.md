# Thiết kế và sơ đồ

Bộ thiết kế tuần 2 phục vụ kế hoạch **6 tuần**:

| Tài liệu | Người phụ trách |
|---|---|
| [hash-bucket.md](hash-bucket.md) | Quang — cấu trúc cache, cấp phát và khóa |
| [timestamp_lru.md](timestamp_lru.md) | Trung — lru_clock, lastuse, brelse/bunpin và chọn nạn nhân |
| [cache_statistics.md](cache_statistics.md) | Trung — nguồn demand/read-ahead, bộ đếm và xem/reset |
| read-ahead.md — Hải cập nhật trong phần bàn giao riêng | Hải — tuần tự, trạng thái file, ánh xạ không cấp phát và ON/OFF |
| [integration.md](integration.md) | Trung tổng hợp — cấu trúc/giao diện/luồng chung và phân công |
| [test_plan.md](../../tests/test_plan.md) | Cả nhóm — ca kiểm thử, chủ sở hữu và minh chứng |

Kế hoạch tuần 2–6 và người đọc chéo: [project-plan.md](../project-plan.md). Thiết kế đã chốt không đồng nghĩa implementation/test đã hoàn thành.

Các bất biến: một ánh xạ cho mỗi (dev, blockno); không thu hồi buffer đang dùng/pin; đồng bộ nội dung đúng; không chờ sleeplock hoặc I/O khi giữ spinlock cache.

Figma/FigJam: **Chưa bổ sung**. Trung tổng hợp sơ đồ/luồng và mỗi người đọc chéo phần mình; không tự coi link hoặc ảnh sơ đồ đã tồn tại.

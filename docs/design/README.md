# Thiết kế và sơ đồ

Bộ thiết kế tuần 2 phục vụ kế hoạch **6 tuần**:

| Tài liệu | Người phụ trách |
|---|---|
| [hash-bucket.md](hash-bucket.md) | Quang — cấu trúc cache, cấp phát và khóa |
| [timestamp_lru.md](timestamp_lru.md) | Trung — lru_clock, lastuse, brelse/bunpin và chọn nạn nhân |
| [cache_statistics.md](cache_statistics.md) | Trung — nguồn demand/read-ahead, bộ đếm và xem/reset |
| [read-ahead.md](read-ahead.md) | Hải — tuần tự, trạng thái file, ánh xạ không cấp phát và ON/OFF |
| [integration.md](integration.md) | Trung tổng hợp — cấu trúc/giao diện/luồng chung và phân công |
| [test_plan.md](../../tests/test_plan.md) | Cả nhóm — ca kiểm thử, chủ sở hữu và minh chứng |

Kế hoạch thực hiện: [project-plan.md](../project-plan.md).

Các bất biến: một ánh xạ cho mỗi (dev, blockno); không thu hồi buffer đang dùng/pin; đồng bộ nội dung đúng; không chờ sleeplock hoặc I/O khi giữ spinlock cache.


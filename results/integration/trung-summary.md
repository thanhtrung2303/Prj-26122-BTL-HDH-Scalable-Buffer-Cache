# Báo cáo kiểm thử tích hợp – Tuần 5

## 1. Môi trường kiểm thử

- SHA tích hợp: `bde8afc85922f103066e27c830fb75f0643d63cc`
- Branch: `main`
- Số CPU QEMU: 3
- NBUF: 30
- READ_AHEAD_BLOCKS: 2

## 2. Kết quả kiểm thử

| Bài kiểm thử | C0 (OFF) | C1 (ON) |
|---|---|---|
| BCACHE SELFTEST | PASS | PASS |
| K01 – Concurrent miss | PASS | PASS |
| S02 – Read-ahead lookup | PASS | PASS |
| S02 – Demand lookup | PASS | PASS |

## 3. Các bộ đếm S02 mong đợi

### Sau `bread_ra()`

- `read_ahead_lookups = 1`
- `read_ahead_lookup_misses = 1`
- `read_ahead_disk_reads = 1`

### Sau `bread()`

- `demand_lookups = 1`
- `demand_lookup_hits = 1`
- `demand_disk_reads = 0`

## 4. Các chỉ tiêu K01 mong đợi

- `arrivals = 2`
- `allocations = 1`
- `rechecks = 1`
- Hai hart sử dụng cùng một buffer.
- `refcnt` cuối cùng bằng 0.

## 5. Kết luận

Cả C0 và C1 đều vượt qua BCACHE SELFTEST, K01 và S02 tại cùng SHA tích hợp.

S02 xác nhận bộ đếm khi gọi trực tiếp `bread_ra()` và khả năng demand lookup tìm thấy dữ liệu trong cache. Bài kiểm thử này chưa đánh giá trực tiếp cơ chế Read-ahead tự động.
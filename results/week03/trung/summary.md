# Minh chứng timestamp LRU tuần 3

Nguyễn Thành Trung chạy các ca kiểm thử trên xv6 RISC-V bằng QEMU ngày 06/11/2026 (giờ Việt Nam). Đối chiếu với [ma trận kiểm thử](../../../tests/test_plan.md).

## Mã nguồn

- [84beb3b](https://github.com/thanhtrung2303/Prj-26122-BTL-HDH-Scalable-Buffer-Cache/commit/84beb3b) thêm kiểm tra tràn `lru_clock` và các ca K05, K06.
- [34299d0](https://github.com/thanhtrung2303/Prj-26122-BTL-HDH-Scalable-Buffer-Cache/commit/34299d0) thêm ca K07 cho tiến trình chờ sleeplock.

## Self-test trên 3 CPU

| Ca | Kết quả quan sát |
| --- | --- |
| K05 | PASS. Cache chọn buffer rảnh có timestamp cũ hơn giữa hai bucket. |
| K06 | PASS. Timestamp giữ nguyên khi vẫn còn pin và được cập nhật khi bỏ pin cuối. |
| K07 | PASS. Tiến trình chờ sleeplock giữ một tham chiếu. Cache không tái sử dụng buffer khi tham chiếu đó còn hiệu lực và cập nhật timestamp sau lần nhả cuối. |

- Lượt 21:21 ghi commit nền `c647987` và diff thử nghiệm trước khi chốt commit K05–K06 với `BCACHE_SELFTEST=1`. xv6 vào shell, sau đó `rwtest` và `cachecheck` đạt. [Log](lru-k05-k06-cpus3.log) · [Ảnh lúc boot](lru-k05-k06-cpus3.png)
- Lượt 22:15 ghi commit nền `84beb3b` và diff của K07 với `BCACHE_SELFTEST=1`. Ba file mã nguồn K07 trong diff khớp các mã blob rút gọn của commit `34299d0`. Log có cả ba dòng PASS K05, K06 và K07, sau đó xv6 vào shell. [Log](lru-k05-k06-k07-cpus3.log) · [Ảnh lúc boot](lru-k05-k06-k07-cpus3.png)

## Hồi quy với self-test tắt

| Cấu hình | Kết quả | Minh chứng |
| --- | --- | --- |
| 1 CPU | `rwtest` khớp 4096 byte. `cachecheck` và `usertests` đạt. `stressfs` quay về shell. | [Boot, SHA và kiểm tra đọc ghi](lru-rwtest-cachecheck-cpus3-and-cpus1.png) · [usertests](lru-usertests-cpus1.png) · [stressfs](lru-stressfs-cpus1.png) |
| 3 CPU | `rwtest` và `cachecheck` đạt trong ảnh. Lượt riêng lúc 21:54 ghi `usertests` đạt và `stressfs` quay về shell. | [Kiểm tra đọc ghi](lru-rwtest-cachecheck-cpus3-and-cpus1.png) · [Log usertests và stressfs](lru-selftest-off-usertests-stressfs-cpus3.log) |

Ảnh của lượt 1 CPU hiển thị SHA `84beb3b`. Hai ảnh tiếp theo ghi phần cuối của `usertests` và lượt `stressfs` trong cùng chuỗi ảnh. Minh chứng hồi quy 3 CPU không ghi SHA. Lượt `usertests` và `stressfs` trên 3 CPU bắt đầu lúc 21:54:59, tách riêng với lượt `rwtest` và `cachecheck` trong ảnh.

Các lượt hồi quy ở trên diễn ra trước K07. Bộ minh chứng hiện chưa có lượt hồi quy sau K07 với `BCACHE_SELFTEST=0` hoặc kết quả hồi quy trên commit tích hợp PR #11.

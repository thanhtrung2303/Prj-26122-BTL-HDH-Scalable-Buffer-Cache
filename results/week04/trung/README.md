# Minh chứng kiểm thử bộ đếm buffer cache tuần 4

Các lượt chạy dùng QEMU với 3 CPU và 128 MiB RAM. Các log có ghi phiên bản đều báo QEMU 8.2.2. Read-ahead tự động tắt ở cả năm lượt. SHA và các thay đổi local dùng khi biên dịch được ghi ở đầu từng log.

| Lượt chạy | SHA nền | Self-test / K01 | Kết quả và ảnh |
| --- | --- | --- | --- |
| [Self-test sau khi gắn bộ đếm](selftest-cpus3.log) | `276b25c` | Bật / bật | [K01, K03, K05–K07](selftest-cpus3-boot.png) và [cachecheck, rwtest](selftest-cpus3-cachecheck-rwtest.png) đạt. |
| [Hồi quy với self-test tắt](regression-off-cpus3.log) | `276b25c` | Tắt / tắt | [cachecheck, rwtest và stressfs](regression-off-cpus3-cachecheck-rwtest-stressfs.png) hoàn tất. [usertests](regression-off-cpus3-usertests.png) báo `ALL TESTS PASSED`. |
| [Snapshot và reset](stats-snapshot-reset-cpus3.log) | `3b4c11e` | Bật / tắt | [Self-test](stats-snapshot-reset-cpus3.png) báo cộng dồn đúng bộ đếm các bucket và reset chỉ xóa thống kê. |
| [S02](s02-selftest-cpus3.log) | `3b4c11e` | Bật / tắt | [Self-test](s02-selftest-cpus3.png) xác nhận read-ahead có một miss và một lượt đọc đĩa, demand sau đó có một hit và không đọc đĩa. Mã S02 lúc chạy là thay đổi local được ghi trong log. |
| [Syscall và lệnh cache_stat](cache-stat-syscall-cpus3.log) | `6175fdc` | Bật / tắt | Có [thống kê trước và sau cachecheck](cache-stat-syscall-cpus3-before-after-cachecheck.png), [reset và kiểm tra tham số sai](cache-stat-syscall-cpus3-reset-invalid.png). Snapshot/reset và S02 cũng đạt trong lượt này. |

Sau `cache_stat reset`, lần mở lại `cache_stat` ghi nhận 35 lượt demand hit, 0 miss và 0 lượt đọc đĩa. Việc khởi chạy lệnh có thể phát sinh lookup trước khi snapshot. S02 gọi trực tiếp `bread_ra()` trong kernel, nên chạy được với read-ahead tự động tắt.

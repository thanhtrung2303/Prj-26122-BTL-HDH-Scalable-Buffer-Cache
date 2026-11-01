# Ma trận kiểm thử và nghiệm thu

- Phạm vi: hash bucket + timestamp LRU + cache statistics + sequential read-ahead.
- Trạng thái ban đầu của toàn bộ ca trong tài liệu: CHƯA CHẠY.
- Các chương trình/test hook mới dưới đây là yêu cầu cần triển khai, không phải lệnh hiện đã có trong repo.

## 1. Cấu hình và nguyên tắc chung

| Mã cấu hình | Nội dung |
|---|---|
| B0 | xv6 nền, không có các thay đổi cache; ghi SHA thực tế, không tự coi nhánh hiện tại là bản nền |
| C0 | Hash bucket + LRU + thống kê; READ_AHEAD_ENABLED=0 |
| C1 | Cùng implementation với C0; READ_AHEAD_ENABLED=1, READ_AHEAD_BLOCKS=2 |

Giữ NBUF=30, NB_BUCKET=13 ở C0/C1; ghi CPUS, FSSIZE và phiên bản công cụ. Chạy regression với CPUS=1 và CPUS=3; ca tranh chấp chạy ít nhất CPUS=3. Dùng bộ dữ liệu và ảnh đĩa đầu vào tương đương giữa các lượt.

Lệnh môi trường hiện có, chạy từ thư mục repo:

```sh
git rev-parse HEAD
git status --short
uname -a
qemu-system-riscv64 --version
riscv64-linux-gnu-gcc --version
make --version
```

Ghi đúng compiler thực sự dùng nếu tool prefix khác. Sau khi đổi cấu hình biên dịch:

```sh
make clean
make qemu CPUS=3
```

Không chạy make clean để xóa ảnh đĩa chứa dữ liệu chưa lưu. Tạo dữ liệu thử có thể tái tạo trong mỗi lượt hoặc dùng bản sao ảnh đầu vào được chuẩn bị trước. Không thay ảnh đĩa đang được QEMU sử dụng.

Trong xv6, các lệnh hiện có:

```sh
usertests
stressfs
```

Sau khi triển khai chương trình test và thống kê, đăng ký vào UPROGS để chạy các lệnh ở mục 2. Kết quả PASS yêu cầu kiểm tra nội dung/điều kiện thực tế, không chỉ in chuỗi cố định.

## 2. Công cụ kiểm thử cần bổ sung

### 2.1. User test: cache_test

Hải phụ trách user/cache_test.c, đăng ký trong Makefile. Giao diện đích:

```sh
cache_test rw
cache_test repeat
cache_test capacity
cache_test concurrent
cache_test sequential
cache_test partial
cache_test large-read
cache_test eof
cache_test indirect
cache_test independent-open
cache_test shared-file
```

Mỗi ca tạo dữ liệu xác định bằng vị trí, ví dụ byte thứ i là `(i * 17 + 3) % 251`, kiểm tra mọi giá trị trả về của open/read/write/fork/wait, so sánh từng byte và số byte, phát hiện EOF đúng vị trí. Không chỉ dùng toàn byte 0 vì dễ bỏ sót lỗi ánh xạ.

Với file vượt cache, chọn 64 block nếu MAXFILE/FSSIZE của commit cho phép; phải lớn hơn NBUF và còn đủ chỗ cho metadata/log. Nếu không đủ, chọn kích thước hợp lệ khác và ghi rõ. indirect cần lớn hơn NDIRECT block nhưng không vượt MAXFILE. Dùng BSIZE/NDIRECT của commit thực tế.

Mỗi ca phải in ID, cấu hình dữ liệu, PASS/FAIL và chi tiết lỗi đầu tiên; trả trạng thái lỗi để chương trình gọi phát hiện. Thu hồi file thử và đợi mọi tiến trình con trước khi kết thúc. Cùng các ca dữ liệu phải chạy được trên B0, không phụ thuộc syscall thống kê mới.

### 2.2. Kernel test có kiểm soát

Quang phụ trách hook thử nghiệm cấp phát/chống trùng; Trung phụ trách LRU/thống kê. Chỉ bật trong bản test bằng cấu hình riêng, mặc định tắt ở bản demo.

Kernel test cần chủ động đồng bộ các bên bằng barrier và kiểm tra bất biến, không phụ thuộc vào may rủi lịch chạy. Dùng block thử thuộc ảnh dữ liệu hợp lệ nếu có I/O; test metadata thuần không gọi driver với thiết bị/block giả.

Ca hết buffer phải chạy trong harness kernel được kiểm soát: giữ các tham chiếu, gọi trực tiếp đường cấp phát read-ahead, kiểm tra trả 0 rồi nhả hết. Không giữ tất cả buffer rồi quay về shell vì shell có thể cần demand và panic ngoài chủ đích.

### 2.3. Đo thống kê

Trung phụ trách cache_stat và syscall theo integration.md. Đối với số đếm chính xác, snapshot trực tiếp trong harness kernel đã cô lập, tránh nhiễu từ exec/shell. User test dùng bộ đếm tổng để phân tích có ghi rõ phạm vi toàn hệ thống.

## 3. Build, boot và baseline

| ID | Cách thực hiện | Tiêu chí đạt | Cấu hình | Người chính | Minh chứng |
|---|---|---|---|---|---|
| B01 | Build sạch, boot đến shell với 1 và 3 CPU | Build không lỗi, shell nhận lệnh, ghi đủ môi trường/SHA | B0/C0/C1 | Cả nhóm trên máy mình | Build + boot log riêng từng người |
| B02 | Chạy usertests đến kết thúc | Có ALL TESTS PASSED; mọi lỗi được đối chiếu test thay vì bỏ qua tùy ý | B0/C0/C1, 1/3 CPU | Quang | Raw log, kết quả tổng hợp |
| B03 | Chạy stressfs | Hoàn tất, trở lại shell, không panic/treo | B0/C0/C1, 3 CPU | Quang | Raw log; không coi đây là test so sánh nội dung |
| B04 | cache_test rw | Ghi, đóng/mở lại, đọc và so sánh toàn bộ dữ liệu; EOF đúng | B0/C0/C1 | Hải | Source, đăng ký UPROGS, log |

B04 là đầu ra test ghi–đọc bắt buộc của Hải cho tuần 2. Log baseline của Quang không thay thế log chạy test mới này.

## 4. Tính đúng dữ liệu và giao diện file

| ID | Cách thực hiện | Tiêu chí đạt | Cấu hình | Người chính |
|---|---|---|---|---|
| D01 | cache_test repeat: đọc cùng file nhiều lượt | Mọi byte/số byte đúng; không suy ra hit tuyệt đối vì có metadata/nhiễu | B0/C0/C1 | Hải |
| D02 | cache_test capacity: file dữ liệu lớn hơn cache | Đọc đủ và đúng sau nhiều lần thay thế; không panic/rò tham chiếu | B0/C0/C1 | Hải |
| D03 | cache_test concurrent: nhiều con đọc chung và file riêng, lặp 20 vòng | Mọi con trả thành công; không sai dữ liệu hoặc treo | B0/C0/C1, 3 CPU | Hải |
| D04 | cache_test sequential: đọc theo BSIZE | Dữ liệu đúng ở cả ON/OFF | B0/C0/C1 | Hải |
| D05 | cache_test partial: đọc 127 byte/lần, gồm đoạn cắt qua ranh giới block | Không mất/lặp byte; đọc nhiều lần trong một block không bị tính thành hai block chuỗi | C0/C1, đối chiếu B0 về dữ liệu | Hải |
| D06 | cache_test large-read: một read nhiều block | Dữ liệu đúng; C1 phát hiện tuần tự trong cùng một read | B0/C0/C1 | Hải |
| D07 | cache_test eof: file 0 byte, BSIZE-1, BSIZE, BSIZE+1; đọc tiếp sau EOF | Số byte đúng; không nạp block ngoài file; không tạo địa chỉ đĩa mới | B0/C0/C1 | Hải |
| D08 | cache_test indirect: file qua NDIRECT | Dữ liệu qua direct/indirect đúng; đọc trước không gọi cấp phát block | B0/C0/C1 | Hải |
| D09 | cache_test independent-open: mở cùng inode hai lần, xen kẽ đọc | Hai offset/lịch sử độc lập; dữ liệu mỗi luồng đúng | B0/C0/C1 | Hải |
| D10 | cache_test shared-file: dup rồi fork, chia sẻ file description | Không trùng/mất vùng đọc do offset; gộp kết quả con để kiểm tra toàn bộ vùng đã đọc | B0/C0/C1 | Hải |

Kiểm tra kích hoạt/không cấp phát ở D05–D08 cần hook trace hoặc kiểm tra kernel; chỉ nhìn output dữ liệu user không đủ chứng minh thuật toán bên trong. Với D10, dùng dữ liệu có ID block và cơ chế tập hợp kết quả để kiểm tra tập vùng đã đọc; không mặc định tiến trình nào nhận block cụ thể.

## 5. Hash bucket, đồng bộ và LRU

Các ca dưới là kernel test có kiểm soát, chạy C0 và C1, CPUS=3.

| ID | Cách thực hiện | Tiêu chí đạt | Người chính |
|---|---|---|---|
| K01 | Buộc hai bên cùng trượt lần tìm đầu cho một khóa, đồng bộ trước đường miss | Chỉ một ánh xạ; một cấp phát miss, bên tìm lại trúng được tính hit | Quang |
| K02 | Yêu cầu các khóa khác nhau có cùng cache_hash | Dữ liệu/định danh không lẫn; so sánh cả dev và blockno | Quang |
| K03 | Ép nạn nhân ở cùng bucket và ở bucket khác | Hnext không vòng, không mất buffer; mỗi mapped xuất hiện đúng một lần | Quang |
| K04 | Còn buffer chưa mapped cùng các buffer mapped rảnh | Ưu tiên chưa mapped, phá hòa theo chỉ số mảng | Quang |
| K05 | Tạo hai buffer rảnh ở hai bucket, lastuse=100 và 30 trong fixture kiểm soát | Chọn timestamp 30 dù bucket của nó có chỉ số lớn hơn | Trung |
| K06 | Lấy buffer, pin; brelse từ 2 về 1; bunpin từ 1 về 0 | Chưa cập nhật ở 2→1; cập nhật lastuse ở 1→0; không chọn khi còn pin | Trung |
| K07 | Nhiều bên giữ/chờ cùng buffer | Refcnt tính cả bên chờ; chỉ lần về 0 cấp timestamp mới | Trung |
| K08 | Dựng hai timestamp bằng nhau trong fixture | Chọn chỉ số mảng nhỏ hơn; fixture sửa metadata dưới khóa, không ở bản production | Trung |
| K09 | Giữ mọi buffer trong harness, yêu cầu bread_ra cho khóa chưa có | Trả 0, một skip, không panic vì thiếu buffer, không rò khóa/refcnt | Quang |
| K10 | Xen kẽ miss, release/unpin, snapshot/reset trong 20 vòng đồng thời | Không deadlock, cấu trúc/refcnt hợp lệ sau mỗi vòng; watchdog phát hiện treo | Quang + Trung |

Fixture không tự sửa metadata khi tiến trình khác đang dùng cache. K09 phải nhả các buffer trước khi cho hoạt động nền tiếp tục. K10 cần thời hạn watchdog ghi trước trong log; quá hạn là TIMEOUT, không được ghi PASS.

## 6. Phát hiện tuần tự và đọc trước

| ID | Cách thực hiện | Tiêu chí đạt | Người chính |
|---|---|---|---|
| R01 | Trace đọc block 4 rồi 5 đến ranh giới | Chưa kích hoạt sau block 4; sau block 5 xét 6 rồi 7 | Hải |
| R02 | Trace nhiều phần nhỏ trong cùng block và read lớn qua nhiều block | Không tăng sai ra_seq_blocks; vẫn kích hoạt trong một read lớn | Hải |
| R03 | Fixture gọi đường đọc có state tại offset không liên tục | Reset chuỗi; không kích hoạt dựa trên lịch sử cũ | Hải |
| R04 | Chạy hết file, read lỗi/0 byte và ghi qua cùng file description | Reset theo đặc tả; read 0 byte không tạo chuỗi mới; lỗi read-ahead không đổi kết quả demand thành lỗi | Hải |
| R05 | Cho block mục tiêu đã valid hoặc đang được bên khác nạp | Tái sử dụng ánh xạ, chỉ một I/O cho lần nạp; giải phóng đủ tham chiếu | Hải + Quang |
| R06 | Đi qua indirect, cuối file và thiếu địa chỉ block trong fixture | Không balloc/ghi bảng ánh xạ do đọc trước; metadata RA mang đúng nguồn | Hải |
| R07 | Thiếu buffer khi tra metadata và khi lấy dữ liệu đọc trước | Dừng cửa sổ, không rò buffer metadata; demand đã thành công vẫn được trả | Hải + Quang |
| R08 | Cùng workload trên C0/C1, thu trace source | C0 không phát READ_AHEAD; C1 chỉ xét tối đa 2 block dữ liệu mỗi lần kích hoạt | Hải |

R03 không yêu cầu lseek ở user space vì xv6 nền không có giao diện đó. R06 không giả định xv6 hỗ trợ tạo sparse file bằng lệnh user; dùng fixture nội bộ nếu cần xét địa chỉ 0.

## 7. Bộ đếm, snapshot và reset

Trung phụ trách; Hải đọc chéo nhãn nguồn. Dùng fixture kernel kiểm soát để kiểm tra số đếm chính xác.

| ID | Cách thực hiện | Tiêu chí đạt |
|---|---|---|
| S01 | Demand cấp phát X, đọc xong rồi demand lại X | Một miss, một hit cho hai lookup; chỉ một disk read nếu fixture bắt đầu lạnh và không có bên khác |
| S02 | bread_ra nạp Y rồi demand Y | RA miss/RA disk read; demand hit; không đổi demand thành nguồn RA |
| S03 | Tìm nhanh trượt, tìm lại có ánh xạ | Một lookup hit, không tăng thêm miss |
| S04 | Ánh xạ có valid==0, tranh sleeplock | Hit vẫn hợp lệ; I/O tính cho đúng bên thực sự phát lệnh; không đếm đôi |
| S05 | Read-ahead không có buffer | RA lookup +1, skip +1; miss và disk read cho khóa bị bỏ không tăng |
| S06 | RA đọc indirect metadata | Lookup/I/O metadata thuộc READ_AHEAD; không ghi vào demand |
| S07 | Gọi bwrite cho buffer đã chuẩn bị | Không tăng disk_reads chỉ vì lệnh ghi |
| S08 | Snapshot/reset đồng thời với lookup | Luôn giữ hai đẳng thức tổng ở dưới; không copyout khi giữ spinlock |
| S09 | Reset trong fixture có I/O đang chạy | Không hủy I/O; chấp nhận lookup/I/O rơi vào hai khoảng; không diễn giải sai thành yêu cầu trọn vẹn |
| S10 | cachereset và cachestat với địa chỉ hợp lệ/không hợp lệ | Kết quả syscall đúng; reset không đổi dữ liệu, LRU hoặc lru_clock; con trỏ sai trả -1 |
| S11 | Chạy cache_stat và cache_stat reset trong shell | In đủ chỉ số, tỷ lệ/N/A đúng, phát hiện lỗi syscall; không đòi số 0 tuyệt đối khi có nhiễu sau reset |

Đẳng thức:

```text
demand_lookups = demand_lookup_hits + demand_lookup_misses
read_ahead_lookups = read_ahead_lookup_hits
                   + read_ahead_lookup_misses + read_ahead_skips
```

Không đặt tiêu chí disk_reads == misses; không dùng read_ahead_lookup_hits làm tỷ lệ đọc trước hữu ích.

## 8. So sánh hiệu năng và giới hạn

Quang chạy workload, Trung tổng hợp thống kê, Hải kiểm tra dữ liệu. Chạy tối thiểu 5 lượt mỗi cấu hình, giữ nguyên điều kiện đầu vào, ghi từng mẫu và median; báo đủ kết quả xấu, không chỉ chọn lượt tốt nhất.

| ID | So sánh | Cách chạy | Số liệu |
|---|---|---|---|
| P01 | B0 với C0 | Cùng workload concurrent, cùng 1/3 CPU và dữ liệu | Thời gian, tính đúng dữ liệu, số CPU; không dùng chênh lệch thời gian đơn lẻ để khẳng định giảm contention |
| P02 | C0 với C1 | Đọc tuần tự cùng file vượt cache | Thời gian, demand/RA/total disk_reads, lookup hit/miss/skip |
| P03 | C0 với C1 | Đọc lặp, file ngắn/EOF, concurrent | Thời gian, dữ liệu đúng, overhead/skip và tổng I/O |
| P04 | Khóa cache ở B0/C0 | Bản đo riêng có instrumentation thử lấy khóa thất bại cho khóa cache | Tổng số vòng thử lấy khóa thất bại và số lần acquire, ghi riêng bucket/eviction/lru_clock |

Đo thời gian bằng uptime theo tick ở test user hoặc đồng hồ host của harness; ghi phương pháp và đơn vị, không gọi tick là millisecond nếu chưa xác định chu kỳ timer của commit. Nếu fixture quá ngắn, tăng số vòng để tránh độ phân giải tick chi phối.

P04 là công cụ đo cần bổ sung ở bản thử nghiệm, không nằm trong chín bộ đếm cache hiện tại. Nếu instrument spinlock, áp dụng cùng cách cho B0/C0, snapshot ngoài vùng giữ khóa và báo overhead. Không claim contention giảm khi chưa có số đo tương ứng. Báo thêm chi phí khóa đồng hồ toàn cục và miss khóa toàn bộ bucket.

Reset thống kê không làm cache lạnh. Tách lượt cold-cache (boot lại, chuẩn bị đầu vào như nhau) và warm-cache (chạy trước một lượt, ghi rõ). User program/shell tạo thêm I/O; thống kê là toàn hệ thống. Không khẳng định RA đồng bộ tăng tốc nếu tổng thời gian không cải thiện.

Không đặt ngưỡng cải thiện phần trăm khi chưa có yêu cầu giảng viên; nếu cần chứng minh mục tiêu hiệu năng, nhóm phải chốt workload và ngưỡng trước khi nghiệm thu, không chọn ngưỡng sau khi thấy kết quả.

## 9. Lưu minh chứng

Đầu ra đích:

```text
results/baseline/<nguoi>-<test>-cpus<N>.log
results/integration/<sha>-<config>-<id>-run<N>.log
results/integration/environment.md
results/integration/summary.md
```

Mỗi log có SHA đầy đủ, tình trạng working tree, cấu hình B0/C0/C1, CPU, ON/OFF, kích thước dữ liệu, lệnh, thời gian bắt đầu/kết thúc và kết quả. Nếu có thay đổi chưa commit, ghi rõ; không gán kết quả cho SHA sạch khác nội dung chạy.

Summary ghi: ID, người chạy, kết quả PASS/FAIL/TIMEOUT/CHƯA CHẠY, link log, lỗi/issue liên quan và người xử lý. Ảnh chỉ bổ sung, raw log là minh chứng chính. Lưu cả log thất bại; không xóa lỗi rồi ghi lại PASS cho cùng lượt.


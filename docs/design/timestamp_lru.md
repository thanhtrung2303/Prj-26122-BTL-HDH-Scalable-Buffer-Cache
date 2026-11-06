# Timestamp LRU Design

- Người phụ trách: Nguyễn Thành Trung.
- Giai đoạn: Thiết kế tuần 2.
- Trạng thái: Đặc tả để triển khai; chưa xác nhận mã kernel hoặc kiểm thử đã hoàn thành.

## 1. Mục tiêu

Thay danh sách LRU toàn cục bằng timestamp trên từng buffer, dùng đúng cấu trúc hash bucket và giao thức khóa trong thiết kế của Quang.

Khi cần thay thế, chỉ chọn buffer có `refcnt == 0`; trong các buffer đã ánh xạ và đủ điều kiện, chọn buffer có timestamp nhỏ nhất trên toàn bộ cache. Demand và read-ahead dùng cùng chính sách thay thế.

Timestamp biểu diễn thời điểm buffer trở về trạng thái không còn người giữ, không phải thời gian thực và không phải thời điểm bắt đầu đọc đĩa.

## 2. Cấu trúc dữ liệu

Các trường liên quan trong `struct buf`:

| Trường | Kiểu | Vai trò |
|---|---|---|
| `dev`, `blockno` | `uint` | Khóa định danh block |
| `refcnt` | `uint` | Số tham chiếu, gồm cả pin và bên đang chờ sleeplock |
| `mapped` | `int` | Buffer đã có ánh xạ hợp lệ hay chưa |
| `hnext` | `struct buf *` | Liên kết trong danh sách hash bucket |
| `lastuse` | `uint64` | Timestamp lần gần nhất refcnt chuyển về 0 |
| `lock` | `struct sleeplock` | Bảo vệ quyền sử dụng nội dung buffer |
| `valid`, `disk`, `data` | Giữ kiểu hiện có | Tiếp tục phục vụ cache và driver đĩa |

Không dùng `prev`/`next` để duy trì danh sách LRU; không thêm danh sách nạn nhân riêng. Buffer chưa ánh xạ nằm trong mảng cố định `bcache.buf[]`, chưa nằm trong bucket nào.

Chốt tên trong đặc tả tích hợp: `bcache.bucket[NB_BUCKET]`, mỗi phần tử có `lock` và `struct buf *head`; danh sách nối qua `hnext`. `head` ở đây là con trỏ đầu danh sách băm, không phải sentinel của danh sách LRU cũ.

Bổ sung clock dùng chung trong `kernel/bio.c`:

```c
static uint64 lru_clock;
static struct spinlock lru_clock_lock;
```

`lru_clock_lock` dùng để bảo vệ clock; `lastuse` của buffer đã ánh xạ vẫn thuộc quyền bảo vệ của khóa bucket.

## 3. Nguồn timestamp và khởi tạo


Trong `binit()`, trước khi cache có người sử dụng:

1. Khởi tạo `eviction_lock`, các khóa bucket và sleeplock buffer theo thiết kế Quang.
2. Khởi tạo `lru_clock_lock`, đặt `lru_clock = 0`.
3. Với mọi buffer, đặt `mapped = 0`, `refcnt = 0`, `valid = 0`, `lastuse = 0`, `hnext = 0`.
4. Các danh sách bucket rỗng.

Mỗi lần hợp lệ làm `refcnt` chuyển từ 1 về 0, tăng đồng hồ một đơn vị và gán giá trị mới cho `lastuse`. Đồng hồ không tăng theo giây hoặc chu kỳ CPU.

Timestamp 0 không được dùng để so sánh buffer chưa ánh xạ. Buffer vừa được cấp phát ánh xạ mới có `refcnt = 1`, `lastuse = 0`, nên chưa đủ điều kiện làm nạn nhân. Lần giải phóng cuối cùng sẽ gán timestamp dương.

Phiên bản đầu dùng `uint64`, không hỗ trợ chạy qua thời điểm tràn 2^64 sự kiện. Hàm tăng phải phát hiện giá trị tối đa và báo lỗi rõ ràng thay vì âm thầm quay về 0; không so sánh timestamp đã tràn bằng phép nhỏ hơn thông thường. Không reset clock khi reset thống kê.

## 4. Khóa bảo vệ và thứ tự lấy khóa

| Khóa | Dữ liệu/thao tác được bảo vệ |
|---|---|
| `bcache.eviction_lock` | Tuần tự hóa cấp phát/chuyển ánh xạ; quản lý buffer chưa ánh xạ |
| `bcache.bucket[i].lock` | Danh sách bucket i; ánh xạ, refcnt, lastuse của buffer thuộc bucket i |
| `lru_clock_lock` | Đọc/tăng lru_clock và cấp timestamp |
| `b->lock` | Nội dung, kiểm tra valid và đọc/ghi dữ liệu theo giao thức xv6 |

Thứ tự khi cần nhiều spinlock:

```text
eviction_lock -> bucket[0] -> ... -> bucket[12] -> lru_clock_lock
```

Đây là thứ tự tương đối: `brelse()`/`bunpin()` chỉ lấy một khóa bucket rồi `lru_clock_lock`; không phải lấy eviction lock hay toàn bộ bucket.

Các quy tắc bắt buộc:

- Không lấy `eviction_lock` khi đang giữ khóa bucket.
- Không lấy bất kỳ khóa bucket nào trong khi đang giữ `lru_clock_lock`.
- Khi lấy nhiều bucket, lấy theo chỉ số tăng dần, nhả theo chỉ số giảm dần.
- Không giữ spinlock khi gọi `acquiresleep()` hoặc thực hiện I/O.
- Khóa driver giữ nguyên giao thức hiện có; không giữ khóa cache qua lời gọi driver.
- Bộ đếm thống kê dùng khóa bucket, không lấy thêm khóa thống kê khi giữ đồng hồ.

Hàm cập nhật timestamp được gọi khi caller đang giữ khóa bucket. Hàm lấy `lru_clock_lock`, tăng đồng hồ, gán `b->lastuse`, nhả khóa đồng hồ; caller tiếp tục giữ khóa bucket đến khi hoàn tất thay đổi refcnt/metadata.

## 5. Quy tắc cập nhật lastuse

### 5.1. Cache hit và chờ sleeplock

Trong `bget()`, tăng `refcnt` dưới khóa bucket trước khi nhả khóa và lấy sleeplock. Không cập nhật `lastuse` tại cache hit: buffer đang có người giữ không được chọn làm nạn nhân.

Nếu nhiều bên dùng chung block, chỉ lần giải phóng cuối cùng làm `refcnt` về 0 mới cấp timestamp mới.

### 5.2. brelse()

1. Kiểm tra caller đang giữ sleeplock.
2. Nhả sleeplock như xv6.
3. Tính bucket từ `dev`, `blockno`, lấy khóa bucket.
4. Kiểm tra `refcnt > 0`, rồi giảm `refcnt`.
5. Nếu giá trị mới là 0, cấp timestamp mới và gán `lastuse`.
6. Nhả khóa bucket.

Caller vẫn giữ một tham chiếu cho đến bước giảm refcnt; vì vậy buffer không thể bị chuyển bucket trong khoảng tính bucket và lấy khóa. Sau khi giảm về 0, việc gán timestamp được bảo vệ bởi khóa bucket, nên eviction chưa thể quan sát trạng thái rảnh với timestamp chưa cập nhật.

### 5.3. bpin()

Caller phải đang có tham chiếu hợp lệ đến buffer. Lấy khóa bucket, tăng `refcnt`, nhả khóa. Không cập nhật timestamp.

### 5.4. bunpin()

Caller phải giữ một pin hợp lệ. Lấy khóa bucket, kiểm tra `refcnt > 0`, giảm refcnt. Nếu giá trị mới là 0, cấp timestamp mới rồi mới nhả khóa bucket.

Ví dụ buffer còn một pin cuối cùng: `brelse()` từ 2 xuống 1 không cập nhật; `bunpin()` từ 1 xuống 0 phải cập nhật. Nếu bỏ qua trường hợp này, buffer vừa hoàn tất commit có thể bị coi là cũ dù mới trở lại trạng thái rảnh.

### 5.5. Read-ahead

Buffer do `bread_ra()` lấy cũng được giải phóng qua `brelse()`, nhận timestamp theo cùng quy tắc. Metadata indirect do Hải đọc trước cũng tuân thủ quy tắc này.

Không cập nhật timestamp riêng trong `readi_ra()` hoặc `bmap_lookup_ra()`. Không phân biệt ưu tiên LRU theo nguồn yêu cầu trong phiên bản đầu.

## 6. Chọn buffer thay thế

Chỉ chạy đường cấp phát sau khi lần tìm nhanh không thấy ánh xạ:

1. Nhả khóa bucket đích.
2. Lấy `eviction_lock`, rồi lấy toàn bộ khóa bucket từ 0 đến 12.
3. Tìm lại `(dev, blockno)` trong bucket đích. Nếu đã có, tăng refcnt và dùng buffer đó, không thay thế.
4. Nếu vẫn chưa có, quét mảng `bcache.buf[]` theo chỉ số tăng dần.
5. Ưu tiên buffer `mapped == 0 && refcnt == 0`; chọn chỉ số mảng nhỏ nhất nếu có nhiều buffer.
6. Nếu không còn buffer chưa ánh xạ, chọn trong các buffer `mapped == 1 && refcnt == 0` giá trị `lastuse` nhỏ nhất trên toàn bộ cache.
7. Nếu timestamp bằng nhau, chọn chỉ số trong `bcache.buf[]` nhỏ nhất. Không dùng địa chỉ ngẫu nhiên hoặc thứ tự hash để phá hòa.

Toàn bộ bucket đã khóa nên `refcnt`, ánh xạ và timestamp không đổi trong khi quét. Không cần lấy `lru_clock_lock` để đọc `lastuse`; khóa bucket đã bảo vệ các trường này.

Không chọn buffer đầu tiên tìm được ở bucket số nhỏ. Ví dụ buffer bucket 0 có `lastuse = 100`, bucket 8 có `lastuse = 30`, cả hai rảnh: phải chọn buffer timestamp 30.

## 7. Chuyển buffer và trường hợp không còn buffer

Với nạn nhân đã ánh xạ:

1. Gỡ khỏi danh sách bucket cũ qua `hnext`.
2. Gán `dev`, `blockno` mới; đặt `mapped = 1`, `valid = 0`, `refcnt = 1`, `lastuse = 0`.
3. Chèn vào bucket đích theo hàm băm của Quang.
4. Nhả mọi khóa bucket theo chiều giảm dần, rồi nhả eviction lock.
5. Sau đó mới lấy sleeplock và trả buffer.

Buffer chưa ánh xạ không có bước gỡ danh sách. Nếu bucket nguồn và đích trùng nhau, khóa đó vẫn chỉ được lấy một lần. Không sửa dữ liệu hoặc trạng thái driver tùy tiện trong lúc đổi ánh xạ.

Khi tất cả buffer đều đang được giữ/pin:

- Demand: nhả các spinlock rồi giữ hành vi `panic("bget: no buffers")` của phương án Quang.
- Read-ahead: nhả các spinlock, trả `0` từ đường lấy buffer tùy chọn và dừng cửa sổ đọc trước của Hải.

Không chờ buffer rảnh trong khi giữ eviction lock hoặc khóa bucket; không thu hồi buffer có `refcnt > 0`.

## 8. Ví dụ timestamp

Giả sử `lru_clock = 40`:

| Sự kiện | refcnt sau sự kiện | lastuse | lru_clock |
|---|---:|---:|---:|
| A: brelse từ 1 về 0 | 0 | 41 | 41 |
| B: brelse từ 2 về 1, còn một pin | 1 | Giữ nguyên | 41 |
| C: brelse từ 1 về 0 | 0 | 42 | 42 |
| B: bunpin từ 1 về 0 | 0 | 43 | 43 |

Nếu không có buffer chưa ánh xạ, trong A/B/C chọn A vì timestamp 41 nhỏ nhất. Nếu A đang được dùng lại và có refcnt 1 thì loại A, chọn C. Không dùng timestamp của buffer đang được giữ để quyết định thay thế.

## 9. Bất biến và kiểm thử

- Buffer đã ánh xạ thuộc đúng một bucket khớp hàm băm; không trùng `(dev, blockno)`.
- Buffer có `refcnt > 0` không được thay thế, kể cả bên đang chờ sleeplock và buffer được pin.
- Mọi chuyển refcnt về 0 ở `brelse()` hoặc `bunpin()` phải đi cùng cập nhật timestamp dưới khóa bucket.
- Nạn nhân đã ánh xạ là buffer rảnh có timestamp nhỏ nhất trên toàn cache.
- Kiểm thử thay thế trong cùng bucket, giữa các bucket, ưu tiên buffer chưa ánh xạ và phá hòa xác định.
- Kiểm thử pin cuối, giải phóng có nhiều người chờ, demand/read-ahead đồng thời và hết buffer.
- Chạy usertests, stressfs và test ghi–đọc so sánh nội dung sau tích hợp. Không ghi PASS khi chưa có log.

## 10. Giao diện phối hợp và giới hạn

- Quang giữ cấu trúc `mapped`, `hnext`, `lastuse` và luồng khóa-all-bucket khi miss; tích hợp quy tắc chọn nạn nhân của tài liệu này.
- Trung bổ sung đồng hồ, khóa đồng hồ và cập nhật ở cả brelse/bunpin; phối hợp thống kê trong `cache_statistics.md`.
- Hải dùng `bread_ra()`/`brelse()`, không tự sửa lastuse hoặc refcnt. Nhãn nguồn đọc được truyền theo từng yêu cầu, không nằm cố định trong buffer.

Đây là LRU theo thời điểm trở về trạng thái rảnh. Khóa đồng hồ toàn cục vẫn có chi phí trên đường giải phóng cuối; miss còn khóa mọi bucket. Chưa có số liệu để khẳng định contention hoặc hiệu năng đã đạt mục tiêu.

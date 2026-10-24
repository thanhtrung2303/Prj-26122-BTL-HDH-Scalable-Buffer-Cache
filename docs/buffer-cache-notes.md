# Khảo sát luồng buffer cache hiện tại

- **Dự án:** 26122 — Scalable buffer cache with read-ahead.
- **Người phụ trách:** Nguyễn Thành Trung — 20233680.
- **Phạm vi:** khảo sát mã nguồn buffer cache hiện tại, phục vụ thiết kế cải tiến.
- **Trạng thái:** bản ghi chú phân tích mã nguồn, chưa có kết quả kiểm thử thực nghiệm kèm theo.

## 1. Khái niệm cơ bản

| Khái niệm | Ý nghĩa trong xv6 |
|---|---|
| Block | Khối dữ liệu của hệ thống file, được xác định bởi thiết bị và số block |
| Buffer | Vùng RAM chứa dữ liệu của một block cùng thông tin quản lý |
| Buffer cache | Tập hợp buffer giữ dữ liệu để giảm số lần đọc đĩa |
| Lock | Cơ chế phối hợp truy cập dữ liệu chung giữa các tiến trình/CPU |
| Reference count | Số tham chiếu giữ buffer, ngăn buffer bị tái sử dụng khi vẫn cần thiết |

Trong phiên bản khảo sát, `BSIZE = 1024` byte, được định nghĩa trong `kernel/fs.h`.

## 2. Cấu trúc `struct buf`

Định nghĩa tại `kernel/buf.h`.

| Trường | Kiểu | Ý nghĩa và cơ chế quản lý |
|---|---|---|
| `valid` | `int` | Cờ đánh dấu trạng thái hợp lệ của dữ liệu (`valid = 1` nghĩa là mảng `data` đã chứa dữ liệu chuẩn xác từ đĩa) |
| `disk` | `int` | Trạng thái I/O: driver đặt 1 khi driver đĩa đang trong quá trình đọc/ghi dữ liệu, đặt 0 khi hoàn tất |
| `dev` | `uint` | Mã thiết bị chứa block; việc gán lại được bảo vệ bởi `bcache.lock` |
| `blockno` | `uint` | Số block trên thiết bị; cùng `dev` xác định block mà buffer đại diện |
| `lock` | `struct sleeplock` | Khóa riêng bảo vệ việc sử dụng nội dung buffer |
| `refcnt` | `uint` | Số tham chiếu đang giữ buffer; thay đổi dưới `bcache.lock` |
| `prev`, `next` | `struct buf *` | Liên kết buffer trong danh sách đôi vòng; thay đổi dưới `bcache.lock` |
| `data` | `uchar[BSIZE]` | Nội dung block; bên gọi truy cập khi giữ khóa buffer, driver truyền dữ liệu trong quá trình I/O |

## 3. Vai trò các hàm

Các hàm sau nằm trong `kernel/bio.c`.

| Hàm | Vai trò |
|---|---|
| `binit()` | Khởi tạo khóa chung, khóa từng buffer và danh sách buffer |
| `bget(dev, blockno)` | Tìm hoặc tái sử dụng buffer cho block; trả về buffer đã khóa |
| `bread(dev, blockno)` | Lấy buffer và đọc đĩa nếu dữ liệu chưa hợp lệ |
| `bwrite(b)` | Ghi nội dung buffer xuống đĩa; yêu cầu bên gọi giữ khóa buffer |
| `brelse(b)` | Trả khóa, giảm tham chiếu và cập nhật danh sách khi tham chiếu về 0 |
| `bpin(b)` | Tăng tham chiếu để giữ buffer trong cache |
| `bunpin(b)` | Giảm tham chiếu đã pin |

`bget()` chỉ bảo đảm lấy đúng buffer và giữ khóa. `bread()` bảo đảm thêm rằng dữ liệu đã hợp lệ.

## 4. Luồng đọc block

Bên gọi trong kernel sử dụng theo mẫu:

```c
struct buf *b = bread(dev, blockno);

/* Sử dụng b->data khi đang giữ khóa buffer. */

brelse(b);
```

### 4.1. Tìm thấy buffer và dữ liệu hợp lệ

1. `bread()` gọi `bget()`.
2. `bget()` lấy `bcache.lock`, duyệt danh sách và tìm cặp `(dev, blockno)`.
3. Khi tìm thấy, tăng `refcnt` để giữ buffer.
4. Trả `bcache.lock`, lấy sleeplock của buffer rồi trả về.
5. `bread()` thấy `valid == 1`, bỏ qua thao tác đọc đĩa.
6. Bên gọi sử dụng dữ liệu và gọi `brelse()` khi hoàn tất.

### 4.2. Chưa có buffer cho block cần đọc

1. `bget()` không tìm thấy cặp `(dev, blockno)`.
2. Duyệt từ cuối danh sách, chọn buffer đầu tiên có `refcnt == 0`.
3. Gán `dev`, `blockno` mới; đặt `valid = 0`, `refcnt = 1`.
4. Trả khóa chung, lấy khóa buffer rồi trả về `bread()`.
5. `bread()` gọi `virtio_disk_rw(b, 0)` để đọc dữ liệu vào `b->data`.
6. Sau khi I/O hoàn tất, đặt `valid = 1` và trả buffer cho bên gọi.

Nếu không có buffer đủ điều kiện, `bget()` gọi `panic("bget: no buffers")`.

### 4.3. Tìm thấy buffer nhưng dữ liệu chưa hợp lệ

Một buffer có thể đã được gán cho block nhưng chưa hoàn tất đọc dữ liệu. Vì vậy, tìm thấy buffer trong `bget()` không đồng nghĩa có thể dùng ngay dữ liệu.

Sau khi lấy được khóa buffer, `bread()` kiểm tra lại `valid`. Nếu vẫn bằng 0 thì đọc đĩa; nếu một tiến trình trước đã đọc xong và đặt bằng 1 thì dùng dữ liệu đó.

## 5. Khóa và biến đếm tham chiếu

### Khóa chung `bcache.lock` (Spinlock)

Bảo vệ việc tìm, gán buffer, cập nhật `refcnt` và danh sách. Việc kiểm tra và gán block dưới cùng khóa giúp tránh hai buffer đại diện cho cùng một block.

Giúp tránh xung đột danh sách liên kết khi nhiều CPU cùng gọi `bget()` hoặc `brelse()` đồng thời.

Thời gian giữ khóa ngắn, các thao tác diễn ra trong vài chu kỳ lệnh máy, giúp tránh context switch tốn kém của OS.

### Khóa riêng `b->lock` (Sleeplock)

Bảo vệ quyền sử dụng dữ liệu `b->data` của một buffer. `bread()` trả về trong trạng thái vẫn giữ khóa; bên gọi trả khóa bằng `brelse()` sau khi xử lý xong.

Đảm bảo chỉ có duy nhất một tiến trình được đọc hoặc sửa mảng `b->data` tại một thời điểm.

Thời gian giữ khóa dài (kéo dài trong suốt quá trình đọc/ghi đĩa cứng I/O hoặc tiến trình đang thao tác dữ liệu).

Với Sleeplock, tiến trình yêu cầu đĩa sẽ đi ngủ (nhường CPU cho tiến trình khác làm việc), và chỉ được đánh thức khi ngắt I/O đĩa xong.

Trong `bget()`, thứ tự là:

```c
b->refcnt++;
release(&bcache.lock);
acquiresleep(&b->lock);
```

Tăng tham chiếu trước giúp buffer không bị tái sử dụng trong lúc bên gọi chờ khóa. Trả spinlock trước khi lấy sleeplock giúp tránh ngủ chờ khi vẫn giữ khóa chung.

### Biến `refcnt`

`refcnt` đếm số lượng tiến trình hoặc thành phần trong kernel đang đồng thời giữ/sử dụng buffer, bao gồm tham chiếu sử dụng/chờ khóa và tham chiếu do pin.

Vai trò chính của `refcnt`: Giúp kernel phân biệt buffer nào đang phục vụ tiến trình và buffer nào đang rảnh để có thể tái sử dụng.

| Thao tác minh họa | `refcnt` |
|---|---:|
| A lấy buffer | 1 |
| B cũng lấy cùng buffer, có thể đang chờ khóa | 2 |
| A trả buffer | 1 |
| B trả buffer | 0 |

`refcnt = 2` không có nghĩa hai tiến trình cùng truy cập dữ liệu. Sleeplock vẫn chỉ cho một bên sử dụng tại một thời điểm.

## 6. Trả và tái sử dụng buffer

`brelse()` kiểm tra quyền giữ khóa, trả sleeplock rồi lấy `bcache.lock` để giảm `refcnt`.

Nếu `refcnt` về 0, buffer được tháo khỏi vị trí cũ và đưa lên đầu danh sách. Dữ liệu vẫn được giữ lại trong RAM.

- `head.next`: phía mới sử dụng.
- `head.prev`: phía lâu không sử dụng.
- `bget()` tìm từ cuối và chọn buffer có `refcnt == 0`.

Do đó, thứ tự thay thế dựa trên danh sách được cập nhật khi `brelse()` làm tham chiếu về 0, không phải timestamp của mọi lần truy cập.

`refcnt == 0` chỉ cho phép xem xét tái sử dụng, không khiến buffer bị xóa ngay.

## 7. Ảnh hưởng của pin/unpin

Cơ chế log cần giữ buffer chứa dữ liệu đã sửa trước khi hoàn tất ghi giao dịch.

- `bpin()` tăng `refcnt`, ngăn tái sử dụng buffer.
- `bunpin()` giảm tham chiếu pin.
- Pin không đồng nghĩa giữ sleeplock.
- `bunpin()` trong phiên bản này không tự di chuyển buffer trong danh sách.

Trong `kernel/log.c`, `log_write()` pin khi thêm block mới vào giao dịch; `install_trans()` bỏ pin trong quá trình cài đặt giao dịch bình thường.

Không được coi buffer vừa được `brelse()` là chắc chắn có thể tái sử dụng: nó vẫn có thể còn tham chiếu pin hoặc tham chiếu khác.

## 8. Sơ đồ luồng đọc

Sơ đồ luồng đọc của buffer cache được thiết kế trên [Figma](https://www.figma.com/board/XfyWpGJ6HHEUFiIF6UFkwP/Project-26122-%E2%80%94-Lu%E1%BB%93ng-%C4%91%E1%BB%8Dc-buffer-cache-xv6?node-id=0-1&t=4q3h46JFOAou4jSb-1).

## 9. Các điểm cần thảo luận tiếp

1. Khi chuyển sang hash bucket, cách bảo đảm duy nhất một buffer cho mỗi `(dev, blockno)` là gì?
2. Cần lấy những khóa nào khi chuyển buffer giữa hai bucket?
3. Timestamp LRU cập nhật ở thời điểm nào, và xử lý pin/unpin ra sao?
4. Hit/miss sẽ được định nghĩa theo việc tìm thấy buffer hay theo việc phải đọc đĩa?
5. Read-ahead tích hợp ở đâu để tránh đọc trùng và ảnh hưởng truy cập thông thường?

Các nội dung trên là câu hỏi cho giai đoạn thiết kế, chưa phải tính năng đã triển khai.

# Thiết kế tích hợp buffer cache và read-ahead

- Phạm vi: Project 26122 — Scalable buffer cache with read-ahead, xv6-riscv.
- Người tổng hợp: Nguyễn Thành Trung.
- Trạng thái: Đặc tả tích hợp tuần 2, chưa phải xác nhận đã triển khai hoặc kiểm thử thành công.

## 1. Phạm vi và tài liệu thành phần

Tài liệu này chốt phần giao tiếp giữa bốn thiết kế, không thay thế nội dung chi tiết:

| Tài liệu | Trách nhiệm |
|---|---|
| `hash-bucket.md` | Quang: cấu trúc bảng băm, cấp phát/chuyển buffer và chống trùng |
| `timestamp_lru.md` | Trung: timestamp logic, cập nhật và chọn nạn nhân |
| `cache_statistics.md` | Trung: phân loại yêu cầu, bộ đếm và xem/reset |
| `read-ahead.md` | Hải: phát hiện tuần tự, ánh xạ không cấp phát và đọc trước |
| `../../tests/test_plan.md` | Cả nhóm: kiểm thử tích hợp và nghiệm thu |

Tên trên là tên đích khi đưa vào repo. Trước khi merge, thống nhất cách viết dấu gạch dưới/gạch nối và cập nhật liên kết tương ứng; không giữ hai bản khác nội dung của cùng một đặc tả.

Nếu tài liệu hash bucket cũ còn ghi “chờ thống nhất”, dùng quyết định ở đây để cập nhật phần đó. Đây là hợp đồng được soạn cho nhóm đọc chéo, không phải bằng chứng mọi thành viên đã xác nhận.

Phiên bản đầu đọc trước đồng bộ tối đa 2 block; không có worker hoặc hàng đợi I/O nền. Hiệu quả và contention phải được đo sau triển khai.

## 2. Cấu trúc dữ liệu chung

### 2.1. Buffer và cache

Giữ các trường I/O hiện có của `struct buf`, thêm/chốt:

```c
int mapped;
struct buf *hnext;
uint64 lastuse;
```

Không dùng danh sách LRU `prev`/`next` cũ. `refcnt` vẫn tính cả tham chiếu đang chờ sleeplock và pin. `mapped` không đồng nghĩa với `valid`: buffer có thể đã có danh tính nhưng chưa có dữ liệu hợp lệ.

Cấu trúc cache đích trong `kernel/bio.c`:

```c
#define NB_BUCKET 13

struct cache_bucket {
  struct spinlock lock;
  struct buf *head;
};

static struct {
  struct spinlock eviction_lock;
  struct cache_bucket bucket[NB_BUCKET];
  struct buf buf[NBUF];
} bcache;

static uint64 lru_clock;
static struct spinlock lru_clock_lock;
static struct cache_stats bucket_stats[NB_BUCKET];
```

`NBUF = 30` theo thiết kế Quang. Hàm băm duy nhất dùng cho mọi đường:

```c
static uint
cache_hash(uint dev, uint blockno)
{
  return ((uint64)blockno + (uint64)dev) % NB_BUCKET;
}
```

`head` là con trỏ đầu danh sách đơn, nối bằng hnext; không phải sentinel. Buffer chưa ánh xạ nằm trong mảng, chưa thuộc bucket nào. Khởi tạo mapped/refcnt/valid/lastuse/hnext bằng 0; khởi tạo các khóa trước khi công bố cache cho CPU khác.

### 2.2. Trạng thái đọc trước

Thêm trực tiếp các trường sau trong `struct file`, đúng tên của thiết kế Hải:

```c
int ra_has_history;
uint ra_next_off;
uint ra_last_block;
uint ra_seq_blocks;
```

Trong hợp đồng `readi_ra()` dưới đây, tham số `state` chính là con trỏ đến `struct file` chứa bốn trường này; không tạo thêm bản sao trạng thái riêng.

Khởi tạo trước khi công bố file descriptor mới. Hai lần open có trạng thái riêng; dup/fork giữ chung file description nên dùng chung trạng thái. Truy cập trạng thái sau khi công bố phải giữ inode lock, gồm cả reset trước khi ghi qua cùng file description.

### 2.3. Thống kê và nguồn yêu cầu

`kernel/cache_stats.h` chứa `enum cache_source { DEMAND, READ_AHEAD }` và đúng chín trường uint64 của `struct cache_stats` trong đặc tả thống kê. Header chỉ chứa kiểu/giao diện dùng chung, không chứa đối tượng bucket_stats hoặc spinlock.

Đọc/ghi các bộ đếm của bucket i dưới `bcache.bucket[i].lock`. Nguồn yêu cầu truyền bằng tham số cục bộ; không lưu nhãn cố định vào buffer hoặc biến global.

## 3. Hợp đồng giao diện

### 3.1. Buffer cache

Giao diện dùng từ các file kernel khác, khai báo ở `kernel/defs.h`:

```c
struct buf *bread(uint dev, uint blockno);
struct buf *bread_ra(uint dev, uint blockno);
void brelse(struct buf *b);
void bpin(struct buf *b);
void bunpin(struct buf *b);
void bcache_stats_snapshot(struct cache_stats *out);
void bcache_stats_reset(void);
```

Bổ sung forward declaration `struct cache_stats;` trong defs.h. `struct buf`, `struct inode`, `struct file` dùng forward declaration theo quy ước hiện có của xv6.

Hàm dùng chung nội bộ, chỉ có trong bio.c:

```c
static struct buf *bget_source(uint dev, uint blockno,
                               enum cache_source source);
static struct buf *bread_source(uint dev, uint blockno,
                                enum cache_source source);
```

| Hàm | Tiền điều kiện | Kết quả/quyền sở hữu |
|---|---|---|
| bread | Không giữ spinlock cache | DEMAND; trả buffer có refcnt và sleeplock, valid==1; hết buffer thì panic sau khi nhả khóa |
| bread_ra | Không giữ spinlock cache | READ_AHEAD; thành công giống bread; hết buffer thì trả 0 |
| bget_source | Nội bộ; source hợp lệ | Thành công trả buffer có refcnt và sleeplock; chưa bảo đảm valid |
| brelse | Caller giữ sleeplock và một tham chiếu | Nhả sleeplock, giảm refcnt; cập nhật timestamp nếu về 0 |
| bpin | Caller đã có tham chiếu hợp lệ | Tăng refcnt dưới khóa bucket |
| bunpin | Caller có một pin hợp lệ | Giảm refcnt; cập nhật timestamp nếu về 0 |

`bread()` và `bread_ra()` chỉ là wrapper chọn source rồi gọi hàm chung. `bget_source()` dùng source để phân loại lookup và quyết định panic/return 0 khi hết buffer. Không tạo thuật toán cấp phát thứ hai.

`bread_source()` sau khi có buffer sleeplock mới kiểm tra valid. Nếu valid==0, ghi nhận disk_reads của source dưới khóa bucket trong thời gian ngắn, nhả spinlock, gọi virtio_disk_rw(b, 0), rồi đặt valid=1. Không tăng lại lookup tại đây.

### 3.2. Tầng file và filesystem

Giữ nguyên readi cho mọi bên gọi cũ; bổ sung:

```c
int readi_ra(struct inode *ip, int user_dst, uint64 dst,
             uint off, uint n, struct file *state);
```

Khai báo ở defs.h. `state` phải khác 0, trỏ đến file thường đang đọc, `state->ip == ip`; caller giữ inode lock. Chỉ fileread trên T_FILE gọi giao diện này. Không cập nhật f->off bên trong readi_ra: fileread tiếp tục cập nhật theo giá trị trả về như bản nền.

Trong fs.c dùng hàm chung nội bộ:

```c
static int readi_common(struct inode *ip, int user_dst,
                        uint64 dst, uint off, uint n,
                        struct file *state);
static uint bmap_lookup_ra(struct inode *ip, uint bn);
```

readi gọi readi_common với state=0. readi_ra gọi readi_common với state là file hiện tại. Giá trị trả về và xử lý lỗi demand giữ hành vi readi nền; read-ahead thất bại do thiếu buffer không biến phần demand đã đọc thành lỗi.

fs.c cần khai báo struct file đầy đủ để truy cập các trường ra_*. Giữ thứ tự include phù hợp phụ thuộc sleeplock của file.h; không sửa header bằng cách include vòng lặp.

bmap_lookup_ra giữ inode lock, trả địa chỉ block có sẵn hoặc 0. Chỉ dùng bread_ra để đọc metadata phục vụ tra cứu đọc trước; giải phóng metadata trước khi lấy buffer dữ liệu. Không gọi balloc hoặc bmap có nhánh cấp phát cho block đọc trước.

### 3.3. Xem/reset thống kê

Giao diện user:

```c
int cachestat(struct cache_stats *out);
int cachereset(void);
```

Chọn số syscall còn trống lúc triển khai; cập nhật syscall.h, syscall.c, sysproc.c, user.h, usys.pl theo cấu trúc repo. Chương trình user/cache_stat.c hỗ trợ `cache_stat` và `cache_stat reset`, được đăng ký trong UPROGS.

copyout chỉ thực hiện sau khi nhả spinlock. Reset chỉ xóa thống kê, không xóa cache, trạng thái đọc trước hoặc lru_clock.

## 4. Bảng khóa chung

| Khóa | Quyền bảo vệ |
|---|---|
| ip->lock, sleeplock | Đọc/sửa inode, offset và trạng thái read-ahead của file theo luồng file thường |
| bcache.eviction_lock | Cấp phát, chuyển ánh xạ; buffer chưa ánh xạ |
| bucket[i].lock | Danh sách, ánh xạ, refcnt, lastuse của buffer trong bucket; bucket_stats[i] |
| lru_clock_lock | Cấp giá trị đồng hồ logic |
| b->lock, sleeplock | Quyền dùng nội dung, kiểm tra valid và I/O |

Thứ tự nhiều spinlock là eviction_lock → bucket tăng dần → lru_clock_lock. brelse/bunpin chỉ cần bucket của buffer rồi khóa đồng hồ. Snapshot/reset lấy bucket tăng dần, không lấy eviction_lock hoặc lru_clock_lock. Nhả nhiều bucket theo chiều giảm dần.

Không giữ bất kỳ spinlock cache nào khi chờ sleeplock, I/O, in console hoặc copyout. Inode sleeplock được giữ qua đường đọc filesystem như xv6. Đường disk_reads được phép lấy bucket ngắn khi đã giữ buffer sleeplock và refcnt; không có đường ngược giữ bucket rồi chờ sleeplock.

## 5. Luồng xử lý tích hợp

### 5.1. Demand hit/miss và giải phóng

1. fileread lấy inode lock, chọn readi/readi_ra.
2. Vòng lặp đọc ánh xạ block demand và gọi bread → bread_source(DEMAND).
3. bget_source tìm dưới bucket đích. Nếu thấy: tăng refcnt và ghi hit/lookup trong cùng critical section; nhả bucket rồi lấy sleeplock.
4. Nếu chưa thấy: nhả bucket, lấy eviction lock rồi tất cả bucket theo chỉ số tăng dần, tìm lại. Nếu có ánh xạ thì ghi một hit; nếu cấp phát được thì ghi một miss.
5. Cấp phát ưu tiên buffer chưa ánh xạ, rồi buffer mapped/refcnt==0 có lastuse nhỏ nhất toàn cache. Timestamp bằng nhau chọn chỉ số mảng nhỏ nhất.
6. Nếu cần chuyển buffer, gỡ hnext cũ, gán ánh xạ mới, mapped=1, refcnt=1, valid=0, lastuse=0, chèn bucket đích; nhả spinlock rồi lấy sleeplock.
7. bread_source chỉ phát I/O và tăng demand_disk_reads nếu valid==0 sau khi có sleeplock.
8. Sao chép dữ liệu; brelse nhả sleeplock, giảm refcnt dưới bucket; nếu về 0 thì gán lastuse=++lru_clock dưới khóa đồng hồ.

### 5.2. Kích hoạt đọc trước

1. Sau phần demand sao chép thành công, cập nhật lịch sử theo offset byte thực tế. Đọc nhiều phần cùng block không tăng số block chuỗi.
2. Khi đã chạm hai block liên tiếp, phần vừa đọc kết thúc tại ranh giới block và chưa EOF, xét k+1 rồi k+2.
3. Không giữ buffer demand khi lấy thêm buffer đọc trước.
4. bmap_lookup_ra kiểm tra giới hạn file và tra địa chỉ không cấp phát. Metadata đi qua bread_ra, source READ_AHEAD.
5. Dữ liệu đi qua bread_ra và cùng giao thức hash bucket/LRU. Hết buffer: ghi skip ở lookup bị từ chối, trả 0 và dừng cửa sổ; không panic.
6. Sau khi có buffer, chỉ đọc đĩa khi chưa valid; tăng read_ahead_disk_reads tại hàm bread chung. Sau đó brelse.
7. Buffer đọc trước không cập nhật lịch sử tuần tự của demand. Demand dùng lại buffer đó vẫn được ghi nguồn DEMAND.

Chốt READ_AHEAD_ENABLED và READ_AHEAD_BLOCKS=2 trong param.h. OFF không gọi tra cứu/đọc trước; khởi động lại và build sạch khi đổi chế độ.

### 5.3. Tình huống đặc biệt

| Tình huống | Quy tắc |
|---|---|
| Hai bên cùng tìm block chưa có | Tìm lại dưới eviction lock để chỉ tạo một ánh xạ |
| Có ánh xạ nhưng valid==0 | Lookup hit; chờ sleeplock và kiểm tra valid, không suy ra ngay có I/O |
| Tất cả buffer được giữ/pin | Demand panic; read-ahead trả 0 sau khi nhả spinlock |
| Pin cuối được bỏ | bunpin cập nhật timestamp nếu refcnt về 0 |
| EOF/block chưa ánh xạ | Dừng cửa sổ; không cấp phát block đĩa mới |
| Đọc trước thất bại cấp phát | Phần demand đã thành công vẫn được trả bình thường |
| Snapshot/reset khi có I/O đang chạy | Nhất quán ở mức sự kiện; lookup và I/O có thể rơi vào hai khoảng reset khác nhau |

## 6. Phạm vi sửa và phân công

| File/nhóm file | Người chính | Người đọc chéo | Nội dung |
|---|---|---|---|
| kernel/buf.h, cấu trúc và cấp phát trong bio.c | Quang | Trung | mapped/hnext/lastuse, hash, recheck, đổi ánh xạ, đường tùy chọn |
| LRU và đồng hồ trong bio.c | Trung | Quang | Khởi tạo, chọn nạn nhân, brelse/bunpin |
| Thống kê trong bio.c, cache_stats.h, syscall, cache_stat.c | Trung | Hải | Nhãn nguồn, bộ đếm, snapshot/reset, UI |
| kernel/file.h, file.c, fs.c | Hải | Trung | Trạng thái, readi_ra, ánh xạ, ON/OFF và reset lịch sử |
| kernel/defs.h, param.h, Makefile | Người phụ trách thay đổi tương ứng | Trung | Khai báo, cấu hình, UPROGS, tránh ghi đè thay đổi chung |
| Test correctness và baseline | Hải, Quang | Trung | Theo tests/test_plan.md |

bio.c là file dùng chung: Quang tích hợp cấu trúc/đường cấp phát trước; Trung tích hợp LRU/thống kê trên cùng nền. Hải gọi API đã chốt, không tự viết một phiên bản hash cache khác. Đây là phân công đề nghị cho giai đoạn triển khai, cần nhóm xác nhận khi nhận việc.

## 7. Thứ tự triển khai và cổng nghiệm thu

1. Cập nhật hash-bucket.md để phản ánh cấu trúc/tên và quyết định nguồn, timestamp, hết buffer ở đây.
2. Lưu baseline: commit, môi trường, CPU, log usertests/stressfs và test ghi–đọc.
3. Tích hợp hash bucket + timestamp LRU; chạy correctness khi read-ahead OFF.
4. Tích hợp nguồn yêu cầu, thống kê và snapshot/reset; kiểm thử đếm và khóa.
5. Tích hợp đọc trước, EOF, direct/indirect, file description dùng chung.
6. Chạy toàn bộ ma trận, so sánh ON/OFF và ghi số đo. Sửa lỗi correctness trước khi đánh giá hiệu năng.

Điều kiện nghiệm thu thiết kế: không còn giao diện mâu thuẫn, mọi chức năng có người phụ trách và ca kiểm thử. Điều kiện nghiệm thu implementation: có mã chạy, log đúng commit và các ca bắt buộc đạt; không dùng độ hoàn chỉnh của tài liệu làm bằng chứng code đã đúng.

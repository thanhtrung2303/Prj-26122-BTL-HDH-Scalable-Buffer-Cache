# Buffer Cache Test Matrix

## 1. Mục tiêu

Kiểm tra tính đúng đắn, khả năng đồng bộ và hiệu năng
của buffer cache trong xv6.

## 2. Danh sách kiểm thử

| ID | Kiểm thử | Cách thực hiện | Kết quả mong đợi |
|---|---|---|---|
| T01 | Build xv6 | Chạy make | Build thành công |
| T02 | Đọc lặp lại | Đọc cùng một file nhiều lần | Dữ liệu chính xác |
| T03 | Ghi rồi đọc | Ghi dữ liệu, đóng file, mở lại và đọc | Nội dung không thay đổi |
| T04 | Vượt dung lượng cache | Đọc file lớn hơn NBUF | Không panic, dữ liệu chính xác |
| T05 | Đọc đồng thời | Nhiều tiến trình đọc cùng file | Không lỗi dữ liệu |
| T06 | Đọc tuần tự | Đọc file từ đầu đến cuối | Nội dung chính xác |
| T07 | Đọc ngẫu nhiên | Đọc nhiều vị trí khác nhau | Nội dung chính xác |
| T08 | EOF | Đọc vượt cuối file | Không đọc dữ liệu ngoài file |
| T09 | Read-ahead ON/OFF | So sánh hai chế độ | Dữ liệu giống nhau |
| T10 | Cache hit/miss | Đọc lại block đã truy cập | Bộ đếm phản ánh đúng |

## 3. Nguyên tắc đánh giá

- Kiểm tra dữ liệu thực tế, không chỉ dựa vào thông báo PASS.
- Ghi lại lỗi build, lỗi chạy và log QEMU.
- Không ghi kết quả PASS nếu chưa thực hiện kiểm thử.
- So sánh hiệu năng chỉ khi có số liệu đo thực tế.

## 4. Kết quả

Kết quả từng bài kiểm thử sẽ được cập nhật
sau khi chạy trên xv6 và lưu trong results/baseline/.

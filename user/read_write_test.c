#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define TEST_FILE "rw_test_data"
#define DATA_SIZE 512

int
main(void)
{
    char write_buf[DATA_SIZE];
    char read_buf[DATA_SIZE];
    int fd;
    int pos = 0;

    printf("=== File Read/Write Test ===\n");

    // Generate sample data
    for (int i = 0; i < DATA_SIZE; i++) {
        write_buf[i] = 'A' + (i % 26);
    }

    // Create file, truncate old content and write new data
    fd = open(TEST_FILE, O_CREATE | O_WRONLY | O_TRUNC);
    if (fd < 0) {
        printf("FAIL: cannot open file for writing\n");
        exit(1);
    }

    int written = write(fd, write_buf, DATA_SIZE);
    close(fd);

    if (written != DATA_SIZE) {
        printf("FAIL: write error, wrote %d bytes\n", written);
        exit(1);
    }

    // Reopen file for reading
    fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0) {
        printf("FAIL: cannot reopen file\n");
        exit(1);
    }

    // Read data safely until all expected bytes are received
    while (pos < DATA_SIZE) {
        int n = read(fd, read_buf + pos, DATA_SIZE - pos);

        if (n < 0) {
            printf("FAIL: read error\n");
            close(fd);
            exit(1);
        }

        if (n == 0) {
            break;
        }

        // Check bounds before updating position or comparing data
        if (pos + n > DATA_SIZE) {
            printf("FAIL: read exceeds buffer boundary\n");
            close(fd);
            exit(1);
        }

        pos += n;
    }

    close(fd);

    // Check total number of bytes read
    if (pos != DATA_SIZE) {
        printf("FAIL: expected %d bytes, got %d bytes\n",
               DATA_SIZE, pos);
        exit(1);
    }

    // Compare every byte
    for (int i = 0; i < DATA_SIZE; i++) {
        if (write_buf[i] != read_buf[i]) {
            printf("FAIL: mismatch at byte %d\n", i);
            exit(1);
        }
    }

    printf("PASS: all %d bytes match\n", DATA_SIZE);
    exit(0);
}

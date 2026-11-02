#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define DATA_SIZE 4096

static char write_buf[DATA_SIZE];
static char read_buf[DATA_SIZE];

int
main(void)
{
  int fd;
  int i;

  printf("=== File Read/Write Test ===\n");

  // Prepare test data.
  for (i = 0; i < DATA_SIZE; i++)
    write_buf[i] = (char)(i % 256);

  // Create/open the test file and write data.
  fd = open("rwtest.dat", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("FAIL: cannot open file for writing\n");
    exit(1);
  }

  if (write(fd, write_buf, DATA_SIZE) != DATA_SIZE) {
    printf("FAIL: write error\n");
    close(fd);
    exit(1);
  }

  close(fd);

  // Reopen the file and read the data back.
  fd = open("rwtest.dat", O_RDONLY);
  if (fd < 0) {
    printf("FAIL: cannot open file for reading\n");
    exit(1);
  }

  if (read(fd, read_buf, DATA_SIZE) != DATA_SIZE) {
    printf("FAIL: read error\n");
    close(fd);
    exit(1);
  }

  close(fd);

  // Compare written data with data read from the file.
  for (i = 0; i < DATA_SIZE; i++) {
    if (write_buf[i] != read_buf[i]) {
      printf("FAIL: data mismatch at byte %d\n", i);
      exit(1);
    }
  }

  printf("PASS: all %d bytes match\n", DATA_SIZE);
  exit(0);
}

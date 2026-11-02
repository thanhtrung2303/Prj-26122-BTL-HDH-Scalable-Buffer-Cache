#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

#define TEST_BLOCKS 4
#define CHUNK 256

static char expected[TEST_BLOCKS * BSIZE];

int
main(void)
{
  int fd;
  int i, n, pos;
  char actual[CHUNK];

  for (i = 0; i < sizeof(expected); i++)
    expected[i] = (char)((i * 17 + i / BSIZE) & 0xff);

  fd = open("ra_test.dat", O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("ra_test: create failed\n");
    exit(1);
  }
  if (write(fd, expected, sizeof(expected)) != sizeof(expected)) {
    printf("ra_test: write failed\n");
    close(fd);
    exit(1);
  }
  close(fd);

  fd = open("ra_test.dat", O_RDONLY);
  if (fd < 0) {
    printf("ra_test: reopen failed\n");
    exit(1);
  }

  pos = 0;
  while ((n = read(fd, actual, sizeof(actual))) > 0) {
    for (i = 0; i < n; i++) {
      if (actual[i] != expected[pos + i]) {
        printf("ra_test: mismatch at byte %d\n", pos + i);
        close(fd);
        exit(1);
      }
    }
    pos += n;
  }
  close(fd);
  unlink("ra_test.dat");

  if (n < 0 || pos != sizeof(expected)) {
    printf("ra_test: short read (%d/%d)\n", pos, sizeof(expected));
    exit(1);
  }

  printf("ra_test: PASS (%d bytes, sequential small reads)\n", pos);
  exit(0);
}
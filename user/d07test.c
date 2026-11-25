#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BSIZE    1024
#define MAX_SIZE (BSIZE + 1)

static char buf[MAX_SIZE];

static int
test_size(int size)
{
  int fd, i, n;

  unlink("d07data");

  for (i = 0; i < size; i++)
    buf[i] = (char)((i * 13 + 7) % 251);

  fd = open("d07data", O_CREATE | O_RDWR);
  if (fd < 0)
    return -1;

  if (write(fd, buf, size) != size) {
    close(fd);
    unlink("d07data");
    return -1;
  }
  close(fd);

  fd = open("d07data", O_RDONLY);
  if (fd < 0) {
    unlink("d07data");
    return -1;
  }

  n = read(fd, buf, MAX_SIZE);
  if (n != size) {
    close(fd);
    unlink("d07data");
    return -1;
  }

  for (i = 0; i < size; i++) {
    if (buf[i] != (char)((i * 13 + 7) % 251)) {
      close(fd);
      unlink("d07data");
      return -1;
    }
  }

  if (read(fd, buf, 1) != 0) {
    close(fd);
    unlink("d07data");
    return -1;
  }

  close(fd);
  unlink("d07data");
  return 0;
}

int
main(void)
{
  int sizes[] = {0, BSIZE - 1, BSIZE, BSIZE + 1};
  int i;

  printf("D07: file sizes 0, BSIZE-1, BSIZE, BSIZE+1\n");

  for (i = 0; i < 4; i++) {
    if (test_size(sizes[i]) < 0) {
      printf("D07 FAIL: size=%d\n", sizes[i]);
      exit(1);
    }
    printf("D07 PASS: size=%d\n", sizes[i]);
  }

  printf("D07 PASS: EOF verified for all sizes\n");
  exit(0);
}

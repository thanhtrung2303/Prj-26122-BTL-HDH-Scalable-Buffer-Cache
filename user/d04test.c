#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define BLOCK_SIZE 1024
#define BLOCKS     16

static char buf[BLOCK_SIZE];

int
main(void)
{
  int fd, i, j, n;
  char value;

  unlink("d04data");
  fd = open("d04data", O_CREATE | O_RDWR);
  if (fd < 0)
    exit(1);

  for (i = 0; i < BLOCKS; i++) {
    for (j = 0; j < BLOCK_SIZE; j++)
      buf[j] = (char)((i * 7 + j) % 251);

    if (write(fd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
      close(fd);
      unlink("d04data");
      exit(1);
    }
  }
  close(fd);

  fd = open("d04data", O_RDONLY);
  if (fd < 0) {
    unlink("d04data");
    exit(1);
  }

  for (i = 0; i < BLOCKS; i++) {
    n = read(fd, buf, BLOCK_SIZE);
    if (n != BLOCK_SIZE) {
      close(fd);
      unlink("d04data");
      exit(1);
    }

    for (j = 0; j < BLOCK_SIZE; j++) {
      value = (char)((i * 7 + j) % 251);
      if (buf[j] != value) {
        close(fd);
        unlink("d04data");
        exit(1);
      }
    }
  }

  n = read(fd, buf, 1);
  close(fd);
  unlink("d04data");

  if (n != 0)
    exit(1);

  printf("D04 PASS: 16 sequential blocks, data and EOF verified\n");
  exit(0);
}

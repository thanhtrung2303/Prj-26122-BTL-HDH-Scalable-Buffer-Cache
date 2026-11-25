#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BSIZE       1024
#define NDIRECT     12
#define TEST_BLOCKS 16

static char buf[BSIZE];

static char
pattern(int block, int offset)
{
  return (char)((block * 17 + offset * 7 + 3) % 251);
}

int
main(void)
{
  int fd, i, j, n;
  struct stat before, after;

  unlink("d08data");

  fd = open("d08data", O_CREATE | O_RDWR);
  if (fd < 0)
    exit(1);

  // Ghi 16 block, bao gồm cả vùng indirect.
  for (i = 0; i < TEST_BLOCKS; i++) {
    for (j = 0; j < BSIZE; j++)
      buf[j] = pattern(i, j);

    if (write(fd, buf, BSIZE) != BSIZE) {
      close(fd);
      unlink("d08data");
      exit(1);
    }
  }
  close(fd);

  fd = open("d08data", O_RDONLY);
  if (fd < 0 || fstat(fd, &before) < 0) {
    if (fd >= 0)
      close(fd);
    unlink("d08data");
    exit(1);
  }

  if (before.size != TEST_BLOCKS * BSIZE) {
    printf("D08 FAIL: unexpected initial size=%ld\n", before.size);
    close(fd);
    unlink("d08data");
    exit(1);
  }

  // Đọc tuần tự từng block qua ranh giới direct/indirect.
  for (i = 0; i < TEST_BLOCKS; i++) {
    n = read(fd, buf, BSIZE);
    if (n != BSIZE) {
      printf("D08 FAIL: read block=%d returned=%d\n", i, n);
      close(fd);
      unlink("d08data");
      exit(1);
    }

    for (j = 0; j < BSIZE; j++) {
      if (buf[j] != pattern(i, j)) {
        printf("D08 FAIL: data mismatch block=%d offset=%d\n", i, j);
        close(fd);
        unlink("d08data");
        exit(1);
      }
    }

    if (i == NDIRECT - 1)
      printf("D08 PASS: direct blocks verified\n");

    if (i == NDIRECT)
      printf("D08 PASS: first indirect block verified\n");
  }

  if (read(fd, buf, 1) != 0) {
    printf("D08 FAIL: EOF check\n");
    close(fd);
    unlink("d08data");
    exit(1);
  }

  if (fstat(fd, &after) < 0 || after.size != before.size) {
    printf("D08 FAIL: file size changed during read\n");
    close(fd);
    unlink("d08data");
    exit(1);
  }

  close(fd);
  unlink("d08data");

  printf("D08 PASS: 16 blocks, indirect boundary, EOF and file size\n");
  exit(0);
}

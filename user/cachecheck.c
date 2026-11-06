#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BLOCK_SIZE 1024
#define BLOCK_COUNT 64
#define READERS 4
#define ROUNDS 3

static char data[BLOCK_SIZE];
static char *filename = "quang-cache.tmp";

static uchar
pattern(int block, int offset)
{
  return (uchar)((block * 37 + offset * 13) % 251);
}

static void
fail(char *message)
{
  fprintf(2, "FAIL: %s\n", message);
  exit(1);
}

static void
verify(void)
{
  for (int round = 0; round < ROUNDS; round++) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
      fail("open for reading");

    for (int block = 0; block < BLOCK_COUNT; block++) {
      int done = 0;

      while (done < BLOCK_SIZE) {
        int n = read(fd, data + done, BLOCK_SIZE - done);
        if (n <= 0)
          fail("short read or read error");
        done += n;
      }

      for (int j = 0; j < BLOCK_SIZE; j++) {
        if ((uchar)data[j] != pattern(block, j))
          fail("data mismatch");
      }
    }

    if (read(fd, data, 1) != 0)
      fail("expected EOF");

    close(fd);
  }
}

int
main(void)
{
  int fd = open(filename, O_CREATE | O_WRONLY | O_TRUNC);
  if (fd < 0)
    fail("create test file");

  for (int block = 0; block < BLOCK_COUNT; block++) {
    for (int j = 0; j < BLOCK_SIZE; j++)
      data[j] = pattern(block, j);

    if (write(fd, data, BLOCK_SIZE) != BLOCK_SIZE)
      fail("write");
  }
  close(fd);

  verify();
  printf("PASS: 64 KiB, 3 repeated reads, EOF\n");

  // Children open the file independently, with separate offsets.
  int started = 0;
  int failed = 0;

  for (int i = 0; i < READERS; i++) {
    int pid = fork();

    if (pid < 0) {
      failed = 1;
      break;
    }

    if (pid == 0) {
      verify();
      exit(0);
    }

    started++;
  }

  for (int i = 0; i < started; i++) {
    int status = -1;
    if (wait(&status) < 0 || status != 0)
      failed = 1;
  }

  if (unlink(filename) < 0)
    fail("remove test file");

  if (failed)
    fail("reader process or fork");

  printf("PASS: 4 readers, each reading 64 KiB 3 times\n");
  printf("CACHECHECK PASSED\n");
  exit(0);
}

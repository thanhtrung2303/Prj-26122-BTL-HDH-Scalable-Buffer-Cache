#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define BASIC_SIZE 4096
#define LARGE_SIZE (64 * 1024)
#define CHUNK_SIZE 512

static char io_buf[CHUNK_SIZE];

int
write_test_file(char *name, int size)
{
  int fd;
  int offset = 0;

  fd = open(name, O_CREATE | O_WRONLY | O_TRUNC);
  if (fd < 0) {
    printf("FAIL: cannot create %s\n", name);
    return -1;
  }

  while (offset < size) {
    int chunk = size - offset;
    int done = 0;

    if (chunk > CHUNK_SIZE)
      chunk = CHUNK_SIZE;

    for (int i = 0; i < chunk; i++)
      io_buf[i] = 'A' + ((offset + i) % 26);

    while (done < chunk) {
      int n = write(fd, io_buf + done, chunk - done);
      if (n <= 0) {
        printf("FAIL: write error in %s\n", name);
        close(fd);
        return -1;
      }
      done += n;
    }

    offset += chunk;
  }

  close(fd);
  return 0;
}

int
verify_test_file(char *name, int size)
{
  int fd;
  int offset = 0;

  fd = open(name, O_RDONLY);
  if (fd < 0) {
    printf("FAIL: cannot open %s for reading\n", name);
    return -1;
  }

  while (offset < size) {
    int want = size - offset;
    int n;

    if (want > CHUNK_SIZE)
      want = CHUNK_SIZE;

    n = read(fd, io_buf, want);
    if (n <= 0) {
      printf("FAIL: read error or unexpected EOF in %s at %d\n",
             name, offset);
      close(fd);
      return -1;
    }

    for (int i = 0; i < n; i++) {
      char expected = 'A' + ((offset + i) % 26);
      if (io_buf[i] != expected) {
        printf("FAIL: mismatch in %s at byte %d\n",
               name, offset + i);
        close(fd);
        return -1;
      }
    }

    offset += n;
  }

  close(fd);
  return 0;
}

int
main(void)
{
  int pid;
  int status = -1;
  int waited;

  printf("=== xv6 Read/Write Tests ===\n");

  // Test 1: Basic file read/write.
  if (write_test_file("rw_test_data", BASIC_SIZE) < 0)
    exit(1);

  if (verify_test_file("rw_test_data", BASIC_SIZE) < 0)
    exit(1);

  printf("PASS: basic read/write (%d bytes)\n", BASIC_SIZE);

  // Test 2: Repeated reads.
  for (int i = 0; i < 5; i++) {
    if (verify_test_file("rw_test_data", BASIC_SIZE) < 0)
      exit(1);
  }

  printf("PASS: repeated reads (5 times)\n");

  // Test 3: Read a file larger than the buffer cache.
  if (write_test_file("ra_data", LARGE_SIZE) < 0)
    exit(1);

  if (verify_test_file("ra_data", LARGE_SIZE) < 0)
    exit(1);

  printf("PASS: large file read (%d bytes)\n", LARGE_SIZE);

  // Test 4: Parent and child read the same file concurrently.
  pid = fork();

  if (pid < 0) {
    printf("FAIL: fork error\n");
    exit(1);
  }

  if (pid == 0) {
    int result = verify_test_file("ra_data", LARGE_SIZE);
    exit(result == 0 ? 0 : 1);
  }

  if (verify_test_file("ra_data", LARGE_SIZE) < 0)
    exit(1);

  waited = wait(&status);

  if (waited != pid || status != 0) {
    printf("FAIL: concurrent read (child status=%d)\n", status);
    exit(1);
  }

  printf("PASS: concurrent read (parent + child)\n");
  printf("=== ALL TESTS PASSED ===\n");

  exit(0);
}

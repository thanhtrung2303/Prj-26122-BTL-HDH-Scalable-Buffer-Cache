#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define CHILDREN   3
#define ROUNDS     20
#define FILE_SIZE  4096
#define CHUNK_SIZE 128

static char buf[CHUNK_SIZE];

static char *names[CHILDREN] = {
  "d03_file0", "d03_file1", "d03_file2"
};

static int
create_test_file(char *name, int seed)
{
  int fd;
  int i, j;
  char data[CHUNK_SIZE];

  unlink(name);
  fd = open(name, O_CREATE | O_RDWR);
  if (fd < 0)
    return -1;

  for (i = 0; i < FILE_SIZE; i += CHUNK_SIZE) {
    for (j = 0; j < CHUNK_SIZE; j++)
      data[j] = (char)((i + j + seed) % 251);

    if (write(fd, data, CHUNK_SIZE) != CHUNK_SIZE) {
      close(fd);
      unlink(name);
      return -1;
    }
  }

  close(fd);
  return 0;
}

static int
check_file(char *name, int seed)
{
  int fd, n, i, j;

  fd = open(name, O_RDONLY);
  if (fd < 0)
    return -1;

  for (i = 0; i < FILE_SIZE; i += CHUNK_SIZE) {
    n = read(fd, buf, CHUNK_SIZE);
    if (n != CHUNK_SIZE) {
      close(fd);
      return -1;
    }

    for (j = 0; j < CHUNK_SIZE; j++) {
      if ((uchar)buf[j] != (uchar)((i + j + seed) % 251)) {
        close(fd);
        return -1;
      }
    }
  }

  n = read(fd, buf, 1);
  close(fd);
  return n == 0 ? 0 : -1;
}

static int
wait_children(int started)
{
  int i, status;
  int failed = 0;

  for (i = 0; i < started; i++) {
    if (wait(&status) < 0)
      failed = 1;
    else if (status != 0)
      failed = 1;
  }

  return failed ? -1 : 0;
}

static int
run_shared_file(void)
{
  int i, pid;
  int started = 0;
  int failed = 0;

  printf("D03: shared file, %d children, %d rounds\n",
         CHILDREN, ROUNDS);

  if (create_test_file("d03_shared", 17) < 0) {
    unlink("d03_shared");
    return -1;
  }

  for (i = 0; i < CHILDREN; i++) {
    pid = fork();
    if (pid < 0) {
      failed = 1;
      break;
    }

    if (pid == 0) {
      int round;
      for (round = 0; round < ROUNDS; round++) {
        if (check_file("d03_shared", 17) < 0)
          exit(1);
      }
      exit(0);
    }

    started++;
  }

  if (wait_children(started) < 0)
    failed = 1;

  // Cleanup runs even if fork or a child test failed.
  if (unlink("d03_shared") < 0)
    failed = 1;

  return failed ? -1 : 0;
}

static int
run_separate_files(void)
{
  int i, pid;
  int started = 0;
  int failed = 0;

  printf("D03: separate files, %d children, %d rounds\n",
         CHILDREN, ROUNDS);

  for (i = 0; i < CHILDREN; i++) {
    if (create_test_file(names[i], 31 + i * 37) < 0) {
      failed = 1;
      break;
    }
  }

  if (!failed) {
    for (i = 0; i < CHILDREN; i++) {
      pid = fork();
      if (pid < 0) {
        failed = 1;
        break;
      }

      if (pid == 0) {
        int round;
        for (round = 0; round < ROUNDS; round++) {
          if (check_file(names[i], 31 + i * 37) < 0)
            exit(1);
        }
        exit(0);
      }

      started++;
    }
  }

  if (wait_children(started) < 0)
    failed = 1;

  // Remove every possible temporary file, including partial setup.
  for (i = 0; i < CHILDREN; i++) {
    if (unlink(names[i]) < 0)
      failed = 1;
  }

  return failed ? -1 : 0;
}

int
main(void)
{
  if (run_shared_file() < 0) {
    printf("D03 FAIL: shared file test (cleanup completed)\n");
    exit(1);
  }

  printf("D03 PASS: shared file\n");

  if (run_separate_files() < 0) {
    printf("D03 FAIL: separate files test (cleanup completed)\n");
    exit(1);
  }

  printf("D03 PASS: separate files\n");
  printf("D03 PASS: all tests (%d rounds)\n", ROUNDS);
  exit(0);
}

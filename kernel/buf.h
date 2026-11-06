struct buf {
  int valid;
  int disk;
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;

  int mapped;
  struct buf *hnext;
  uint64 lastuse;

  uchar data[BSIZE];
};

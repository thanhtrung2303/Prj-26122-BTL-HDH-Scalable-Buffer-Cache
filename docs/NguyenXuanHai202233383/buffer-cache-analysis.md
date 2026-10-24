# Buffer Cache Design Analysis

Author: NguyenXuanHai202233383

## 1. Objectives

The scalable buffer cache should preserve buffer-cache correctness while
reducing contention when multiple processes access different disk blocks.

The design should be evaluated against these goals:

- Correctly identify cached blocks.
- Avoid allocating duplicate buffers for the same device and block.
- Allow independent cache operations to proceed concurrently where safe.
- Preserve correct buffer locking and reference-count behavior.
- Measure cache hits, misses, and performance.

## 2. Hash Buckets

A hash function can map a device and block-number pair to a bucket.

Each bucket can maintain the buffers associated with its hash value.
A per-bucket lock can reduce contention compared with one global lock.

Important correctness considerations:

- Search for an existing buffer before allocating a new one.
- Prevent two concurrent requests from creating duplicate buffers for
  the same device and block.
- Protect bucket lists and metadata with the appropriate locks.
- Define a safe lock order when moving a buffer between buckets.
- Handle a full cache without losing or duplicating buffers.

The implementation must be checked against the project's actual code
and locking rules before these design choices are finalized.

## 3. Timestamp-Based LRU

A timestamp-based replacement policy records when a buffer was most
recently used or released, according to the project's chosen semantics.

When a new buffer is needed, the cache can select an eligible buffer
with the oldest timestamp.

The implementation should ensure that:

- Referenced or busy buffers are not evicted.
- Timestamp updates follow a consistent policy.
- Concurrent timestamp and reference-count updates are synchronized.
- The selected victim is revalidated before reuse.

## 4. Sequential Read-Ahead

When a program reads consecutive blocks, the cache may request a future
block before the program explicitly needs it.

Read-ahead should be evaluated for:

- Detecting consecutive block access.
- Avoiding duplicate in-flight read requests.
- Handling end-of-file and invalid block ranges.
- Avoiding unnecessary I/O for non-sequential workloads.
- Preserving buffer-cache locking and correctness.

Read-ahead is a proposed design area until implementation and test
results are recorded separately.

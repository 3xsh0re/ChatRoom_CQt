#include "m_buffer.h"

namespace mtd {

void Buffer::EnsureWriteableBytes(size_t len) {
  if (PrependableBytes() + WriteAbleBytes() < len) {
    buf_.resize(writer_index_ + len);
  } else {
    size_t readable = ReadeableBytes();
    std::copy(begin() + reader_index_, begin() + writer_index_,
              begin() + kCheapPrepend);
    reader_index_ = kCheapPrepend;
    writer_index_ = reader_index_ + readable;
  }
}

ssize_t Buffer::ReadFd(int fd, int &save_errno) {
  char extra_buf[65536]{};  // 64k

  iovec vec[2];
  const size_t writerable = WriteAbleBytes();
  vec[0].iov_base = BeginWrite();
  vec[0].iov_len = writerable;

  vec[1].iov_base = extra_buf;
  vec[1].iov_len = sizeof(extra_buf);

  const int iovcnt = writerable < sizeof(extra_buf) ? 2 : 1;  // vec len

  const ssize_t n = ::readv(fd, vec, iovcnt);

  if (n < 0) {
    save_errno = errno;
  } else if (n <= writerable) {
    writer_index_ += n;
  } else {
    writer_index_ = buf_.size();
    Append(extra_buf, n - writerable);
  }
  return n;
}

ssize_t Buffer::WriteFd(int fd, int &save_errono) {
  ssize_t n = ::write(fd, Peek(), ReadeableBytes());
  if (n < 0) {
    save_errono = errno;
  }
  return n;
}
}  // namespace mtd

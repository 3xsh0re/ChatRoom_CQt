#ifndef M_BUFFER_H
#define M_BUFFER_H

#include <sys/uio.h>
#include <sys/unistd.h>

#include <string>
#include <vector>

namespace mtd {
class Buffer {
 public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  explicit Buffer(size_t initial_size = kInitialSize)
      : buf_(kCheapPrepend + initial_size),
        reader_index_(kCheapPrepend),
        writer_index_(kCheapPrepend) {}

  size_t ReadeableBytes() const { return writer_index_ - reader_index_; }

  size_t WriteAbleBytes() const { return buf_.size() - writer_index_; }

  size_t PrependableBytes() const { return reader_index_; }

  const char *Peek() const { return begin() + reader_index_; }

  void Retrive(size_t len) {
    if (len < ReadeableBytes()) {
      reader_index_ += len;
    } else {
      RetriveAll();
    }
  }

  void RetriveAll() { reader_index_ = writer_index_ = kCheapPrepend; }

  std::string RetriveAllAsString() { return RetriveAsString(ReadeableBytes()); }

  std::string RetriveAsString(size_t len) {
    Retrive(len);
    return std::string(Peek(), len);
  }

  void EnsureWriteableBytes(size_t len);

  void Append(const char *data, size_t len) {
    EnsureWriteableBytes(len);
    std::copy(data, data + len, BeginWrite());
    writer_index_ += len;
  }

  ssize_t ReadFd(int fd, int &save_errono);

  ssize_t WriteFd(int fd, int &save_errono);

 private:
  char *begin() { return buf_.data(); }

  const char *begin() const { return buf_.data(); }

  char *BeginWrite() { return begin() + writer_index_; }

  const char *BeginWrite() const { return begin() + writer_index_; }

  std::vector<char> buf_;
  size_t reader_index_;
  size_t writer_index_;
};
}  // namespace mtd

#endif
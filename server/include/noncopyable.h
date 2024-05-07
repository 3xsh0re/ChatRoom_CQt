#ifndef NON_COPY_ABLE_H
#define NON_COPY_ABLE_H

namespace mtd {
class noncopyable {
 public:
  noncopyable &operator=(noncopyable const &) = delete;
  noncopyable(noncopyable const &) = delete;

 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};
}  // namespace mtd

#endif
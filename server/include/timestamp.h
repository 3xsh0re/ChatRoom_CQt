#ifndef TIMESTEMP_H
#define TIMESTEMP_H
#include <iostream>
#include <time.h>

namespace mtd
{
  class Timestamp {
    public:
      Timestamp();
      explicit Timestamp(int64_t time);
      static Timestamp Now();
      std::string ToString() const;
    private:
      int64_t micro_seconds_since_epoch_;
  }; 
} // namespace mtd


#endif
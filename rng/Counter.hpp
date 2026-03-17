#pragma once

namespace peanutbutter::rng {

class Counter {
 public:
  virtual ~Counter() = default;

  virtual void Seed(unsigned char* password, int password_length) = 0;
  virtual void Get(unsigned char* destination, int destination_length) = 0;
  virtual unsigned char Get() = 0;
};

}  // namespace peanutbutter::rng

#ifndef BUSEDevice_HPP
#define BUSEDevice_HPP

#include <cstdint>

#include "buse.h"

class BUSEDevice {
  friend int xmp_read(void *buf, u_int32_t len, u_int64_t offset, void *userdata);
  friend int xmp_write(const void *buf, u_int32_t len, u_int64_t offset, void *userdata);
  friend void xmp_disc(void *userdata);
  friend int xmp_flush(void *userdata);
  friend int xmp_trim(u_int64_t from, u_int32_t len, void *userdata);

public:
  virtual int run(const char* devPath, uint64_t capacity);

protected:
  virtual int handleRead(uint64_t offset, uint32_t len, void* buf) = 0;
  virtual int handleWrite(uint64_t offset, uint32_t len, const void* buf) = 0;
  virtual void handleDisc() = 0;
  virtual int handleFlush() = 0;
  virtual int handleTrim(uint64_t offset, uint32_t len) = 0;

private:
  struct buse_operations _aop;
};
#endif

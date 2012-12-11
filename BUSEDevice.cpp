#include "BUSEDevice.hpp"

using namespace std;

int xmp_read(void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
  BUSEDevice* owner = static_cast<BUSEDevice*>(userdata);
  return owner->handleRead(offset, len, buf);
}

int xmp_write(const void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
  BUSEDevice* owner = static_cast<BUSEDevice*>(userdata);
  return owner->handleWrite(offset, len, buf);
}

void xmp_disc(void *userdata)
{
  BUSEDevice* owner = static_cast<BUSEDevice*>(userdata);
  return owner->handleDisc();
}

int xmp_flush(void *userdata)
{
  BUSEDevice* owner = static_cast<BUSEDevice*>(userdata);
  return owner->handleFlush();
}

int xmp_trim(u_int64_t from, u_int32_t len, void *userdata){
  BUSEDevice* owner = static_cast<BUSEDevice*>(userdata);
  return owner->handleTrim(from, len);
}

int BUSEDevice::run(const char* devPath, uint64_t capacity){
  _aop = {xmp_read, xmp_write, xmp_disc, xmp_flush, xmp_trim, capacity};
  return buse_main(devPath, &_aop, this);
}

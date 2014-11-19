#ifndef ODR_H
#define ODR_H
#include "common.h"
#include "get_hw_addrs.h"

#define ODR_PROTOCOL 62239

void run_odr(int unixsock, int rawsock, struct hwa_info *hwahead);

#endif

#if !defined(_LOCAL_LOCK_QUEUE_H_)
#define _LOCAL_LOCK_QUEUE_H_

#include "Common.h"
#include "WRLock.h"

class LocalLockQueue {

  const static int kMaxQueueSize = 256;
  static_assert(kMaxQueueSize > MAX_APP_THREAD * define::kMaxCoro, "XX");

};

#endif // _LOCAL_LOCK_QUEUE_H_

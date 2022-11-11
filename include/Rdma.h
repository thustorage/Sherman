#ifndef _RDMA_H__
#define _RDMA_H__

#define forceinline inline __attribute__((always_inline))

#include <assert.h>
#include <cstring>
#include <infiniband/verbs.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <list>
#include <string>

#include "Debug.h"

#define DCT_ACCESS_KEY 3185
#define UD_PKEY 0x11111111
#define PSN 3185

constexpr int kOroMax = 3;
struct RdmaOpRegion {
  uint64_t source;
  uint64_t dest;
  uint64_t size;

  uint32_t lkey;
  union {
    uint32_t remoteRKey;
    bool is_on_chip;
  };
};

extern int kMaxDeviceMemorySize;

struct RdmaContext {
  uint8_t devIndex;
  uint8_t port;
  int gidIndex;

  ibv_context *ctx;
  ibv_pd *pd;

  uint16_t lid;
  union ibv_gid gid;

  RdmaContext() : ctx(NULL), pd(NULL) {}
};

struct Region {
  uint64_t source;
  uint32_t size;

  uint64_t dest;
};

//// Resource.cpp
bool createContext(RdmaContext *context, uint8_t port = 1, int gidIndex = 3,
                   uint8_t devIndex = 0);
bool destoryContext(RdmaContext *context);

ibv_mr *createMemoryRegion(uint64_t mm, uint64_t mmSize, RdmaContext *ctx);
ibv_mr *createMemoryRegionOnChip(uint64_t mm, uint64_t mmSize,
                                 RdmaContext *ctx);

bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *cq,
                     RdmaContext *context, uint32_t qpsMaxDepth = 128,
                     uint32_t maxInlineData = 0);

bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *send_cq,
                     ibv_cq *recv_cq, RdmaContext *context,
                     uint32_t qpsMaxDepth = 128, uint32_t maxInlineData = 0);

bool createDCTarget(ibv_exp_dct **dct, ibv_cq *cq, RdmaContext *context,
                    uint32_t qpsMaxDepth = 128, uint32_t maxInlineData = 0);
void fillAhAttr(ibv_ah_attr *attr, uint32_t remoteLid, uint8_t *remoteGid,
                RdmaContext *context);

//// StateTrans.cpp
bool modifyQPtoInit(struct ibv_qp *qp, RdmaContext *context);
bool modifyQPtoRTR(struct ibv_qp *qp, uint32_t remoteQPN, uint16_t remoteLid,
                   uint8_t *gid, RdmaContext *context);
bool modifyQPtoRTS(struct ibv_qp *qp);

bool modifyUDtoRTS(struct ibv_qp *qp, RdmaContext *context);


//// Operation.cpp
int pollWithCQ(ibv_cq *cq, int pollNumber, struct ibv_wc *wc);
int pollOnce(ibv_cq *cq, int pollNumber, struct ibv_wc *wc);

bool rdmaSend(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
              ibv_ah *ah, uint32_t remoteQPN, bool isSignaled = false);

bool rdmaSend(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
              int32_t imm = -1);

bool rdmaReceive(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
                 uint64_t wr_id = 0);
bool rdmaReceive(ibv_srq *srq, uint64_t source, uint64_t size, uint32_t lkey);

bool rdmaRead(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
              uint32_t lkey, uint32_t remoteRKey, bool signal = true,
              uint64_t wrID = 0);

bool rdmaWrite(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
               uint32_t lkey, uint32_t remoteRKey, int32_t imm = -1,
               bool isSignaled = true, uint64_t wrID = 0);

bool rdmaFetchAndAdd(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t add,
                     uint32_t lkey, uint32_t remoteRKey);
bool rdmaFetchAndAddBoundary(ibv_qp *qp, uint64_t source, uint64_t dest,
                         uint64_t add, uint32_t lkey, uint32_t remoteRKey,
                         uint64_t boundary = 63, bool singal = true,
                         uint64_t wr_id = 0);

bool rdmaCompareAndSwap(ibv_qp *qp, uint64_t source, uint64_t dest,
                        uint64_t compare, uint64_t swap, uint32_t lkey,
                        uint32_t remoteRKey, bool signal = true,
                        uint64_t wrID = 0);
bool rdmaCompareAndSwapMask(ibv_qp *qp, uint64_t source, uint64_t dest,
                            uint64_t compare, uint64_t swap, uint32_t lkey,
                            uint32_t remoteRKey, uint64_t mask = ~(0ull),
                            bool signal = true);

//// Utility.cpp
void rdmaQueryQueuePair(ibv_qp *qp);
void checkDMSupported(struct ibv_context *ctx);


//// specified
bool rdmaWriteBatch(ibv_qp *qp, RdmaOpRegion *ror, int k, bool isSignaled,
                    uint64_t wrID = 0);
bool rdmaCasRead(ibv_qp *qp, const RdmaOpRegion &cas_ror,
                 const RdmaOpRegion &read_ror, uint64_t compare, uint64_t swap,
                 bool isSignaled, uint64_t wrID = 0);
bool rdmaWriteFaa(ibv_qp *qp, const RdmaOpRegion &write_ror,
                  const RdmaOpRegion &faa_ror, uint64_t add_val,
                  bool isSignaled, uint64_t wrID = 0);
bool rdmaWriteCas(ibv_qp *qp, const RdmaOpRegion &write_ror,
                  const RdmaOpRegion &cas_ror, uint64_t compare, uint64_t swap,
                  bool isSignaled, uint64_t wrID = 0);                 
#endif

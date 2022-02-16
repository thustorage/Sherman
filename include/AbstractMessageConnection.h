#ifndef __ABSTRACTMESSAGECONNECTION_H__
#define __ABSTRACTMESSAGECONNECTION_H__

#include "Common.h"

#define SIGNAL_BATCH 31

class Message;

// #messageNR send pool and #messageNR message pool
class AbstractMessageConnection {

  const static int kBatchCount = 4;

protected:
  ibv_qp *message; // ud or raw packet
  uint16_t messageNR;

  ibv_mr *messageMR;
  void *messagePool;
  uint32_t messageLkey;

  uint16_t curMessage;

  void *sendPool;
  uint16_t curSend;

  ibv_recv_wr *recvs[kBatchCount];
  ibv_sge *recv_sgl[kBatchCount];
  uint32_t subNR;

  ibv_cq *send_cq;
  uint64_t sendCounter;

  uint16_t sendPadding; // ud: 0
                        // rp: ?
  uint16_t recvPadding; // ud: 40
                        // rp: ?

public:
  AbstractMessageConnection(ibv_qp_type type, uint16_t sendPadding,
                            uint16_t recvPadding, RdmaContext &ctx, ibv_cq *cq,
                            uint32_t messageNR);

  void initRecv();

  char *getMessage();
  char *getSendPool();

  uint32_t getQPN() { return message->qp_num; }
};

#endif /* __ABSTRACTMESSAGECONNECTION_H__ */

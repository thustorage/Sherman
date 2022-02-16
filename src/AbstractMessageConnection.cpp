#include "AbstractMessageConnection.h"

AbstractMessageConnection::AbstractMessageConnection(
    ibv_qp_type type, uint16_t sendPadding, uint16_t recvPadding,
    RdmaContext &ctx, ibv_cq *cq, uint32_t messageNR)
    : messageNR(messageNR), curMessage(0), curSend(0), sendCounter(0),
      sendPadding(sendPadding), recvPadding(recvPadding) {

  assert(messageNR % kBatchCount == 0);

  send_cq = ibv_create_cq(ctx.ctx, 128, NULL, NULL, 0);

  createQueuePair(&message, type, send_cq, cq, &ctx);
  modifyUDtoRTS(message, &ctx);

  messagePool = hugePageAlloc(2 * messageNR * MESSAGE_SIZE);
  messageMR = createMemoryRegion((uint64_t)messagePool,
                                 2 * messageNR * MESSAGE_SIZE, &ctx);
  sendPool = (char *)messagePool + messageNR * MESSAGE_SIZE;
  messageLkey = messageMR->lkey;
}

void AbstractMessageConnection::initRecv() {
  subNR = messageNR / kBatchCount;

  for (int i = 0; i < kBatchCount; ++i) {
    recvs[i] = new ibv_recv_wr[subNR];
    recv_sgl[i] = new ibv_sge[subNR];
  }

  for (int k = 0; k < kBatchCount; ++k) {
    for (size_t i = 0; i < subNR; ++i) {
      auto &s = recv_sgl[k][i];
      memset(&s, 0, sizeof(s));

      s.addr = (uint64_t)messagePool + (k * subNR + i) * MESSAGE_SIZE;
      s.length = MESSAGE_SIZE;
      s.lkey = messageLkey;

      auto &r = recvs[k][i];
      memset(&r, 0, sizeof(r));

      r.sg_list = &s;
      r.num_sge = 1;
      r.next = (i == subNR - 1) ? NULL : &recvs[k][i + 1];
    }
  }

  struct ibv_recv_wr *bad;
  for (int i = 0; i < kBatchCount; ++i) {
    if (ibv_post_recv(message, &recvs[i][0], &bad)) {
      Debug::notifyError("Receive failed.");
    }
  }
}

char *AbstractMessageConnection::getMessage() {
  struct ibv_recv_wr *bad;
  char *m = (char *)messagePool + curMessage * MESSAGE_SIZE + recvPadding;

  ADD_ROUND(curMessage, messageNR);

  if (curMessage % subNR == 0) {
    if (ibv_post_recv(
            message,
            &recvs[(curMessage / subNR - 1 + kBatchCount) % kBatchCount][0],
            &bad)) {
      Debug::notifyError("Receive failed.");
    }
  }

  return m;
}

char *AbstractMessageConnection::getSendPool() {
  char *s = (char *)sendPool + curSend * MESSAGE_SIZE + sendPadding;

  ADD_ROUND(curSend, messageNR);

  return s;
}

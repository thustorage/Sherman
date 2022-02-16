#include "Rdma.h"

int pollWithCQ(ibv_cq *cq, int pollNumber, struct ibv_wc *wc) {
  int count = 0;

  do {

    int new_count = ibv_poll_cq(cq, 1, wc);
    count += new_count;

  } while (count < pollNumber);

  if (count < 0) {
    Debug::notifyError("Poll Completion failed.");
    sleep(5);
    return -1;
  }

  if (wc->status != IBV_WC_SUCCESS) {
    Debug::notifyError("Failed status %s (%d) for wr_id %d",
                       ibv_wc_status_str(wc->status), wc->status,
                       (int)wc->wr_id);
    sleep(5);
    return -1;
  }

  return count;
}

int pollOnce(ibv_cq *cq, int pollNumber, struct ibv_wc *wc) {
  int count = ibv_poll_cq(cq, pollNumber, wc);
  if (count <= 0) {
    return 0;
  }
  if (wc->status != IBV_WC_SUCCESS) {
    Debug::notifyError("Failed status %s (%d) for wr_id %d",
                       ibv_wc_status_str(wc->status), wc->status,
                       (int)wc->wr_id);
    return -1;
  } else {
    return count;
  }
}

static inline void fillSgeWr(ibv_sge &sg, ibv_send_wr &wr, uint64_t source,
                             uint64_t size, uint32_t lkey) {
  memset(&sg, 0, sizeof(sg));
  sg.addr = (uintptr_t)source;
  sg.length = size;
  sg.lkey = lkey;

  memset(&wr, 0, sizeof(wr));
  wr.wr_id = 0;
  wr.sg_list = &sg;
  wr.num_sge = 1;
}

static inline void fillSgeWr(ibv_sge &sg, ibv_recv_wr &wr, uint64_t source,
                             uint64_t size, uint32_t lkey) {
  memset(&sg, 0, sizeof(sg));
  sg.addr = (uintptr_t)source;
  sg.length = size;
  sg.lkey = lkey;

  memset(&wr, 0, sizeof(wr));
  wr.wr_id = 0;
  wr.sg_list = &sg;
  wr.num_sge = 1;
}

static inline void fillSgeWr(ibv_sge &sg, ibv_exp_send_wr &wr, uint64_t source,
                             uint64_t size, uint32_t lkey) {
  memset(&sg, 0, sizeof(sg));
  sg.addr = (uintptr_t)source;
  sg.length = size;
  sg.lkey = lkey;

  memset(&wr, 0, sizeof(wr));
  wr.wr_id = 0;
  wr.sg_list = &sg;
  wr.num_sge = 1;
}

// for UD and DC
bool rdmaSend(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
              ibv_ah *ah, uint32_t remoteQPN /* remote dct_number */,
              bool isSignaled) {

  struct ibv_sge sg;
  struct ibv_send_wr wr;
  struct ibv_send_wr *wrBad;

  fillSgeWr(sg, wr, source, size, lkey);

  wr.opcode = IBV_WR_SEND;

  wr.wr.ud.ah = ah;
  wr.wr.ud.remote_qpn = remoteQPN;
  wr.wr.ud.remote_qkey = UD_PKEY;

  if (isSignaled)
    wr.send_flags = IBV_SEND_SIGNALED;
  if (ibv_post_send(qp, &wr, &wrBad)) {
    Debug::notifyError("Send with RDMA_SEND failed.");
    return false;
  }
  return true;
}

// for RC & UC
bool rdmaSend(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
              int32_t imm) {

  struct ibv_sge sg;
  struct ibv_send_wr wr;
  struct ibv_send_wr *wrBad;

  fillSgeWr(sg, wr, source, size, lkey);

  if (imm != -1) {
    wr.imm_data = imm;
    wr.opcode = IBV_WR_SEND_WITH_IMM;
  } else {
    wr.opcode = IBV_WR_SEND;
  }

  wr.send_flags = IBV_SEND_SIGNALED;
  if (ibv_post_send(qp, &wr, &wrBad)) {
    Debug::notifyError("Send with RDMA_SEND failed.");
    return false;
  }
  return true;
}

bool rdmaReceive(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
                 uint64_t wr_id) {
  struct ibv_sge sg;
  struct ibv_recv_wr wr;
  struct ibv_recv_wr *wrBad;

  fillSgeWr(sg, wr, source, size, lkey);

  wr.wr_id = wr_id;

  if (ibv_post_recv(qp, &wr, &wrBad)) {
    Debug::notifyError("Receive with RDMA_RECV failed.");
    return false;
  }
  return true;
}

bool rdmaReceive(ibv_srq *srq, uint64_t source, uint64_t size, uint32_t lkey) {

  struct ibv_sge sg;
  struct ibv_recv_wr wr;
  struct ibv_recv_wr *wrBad;

  fillSgeWr(sg, wr, source, size, lkey);

  if (ibv_post_srq_recv(srq, &wr, &wrBad)) {
    Debug::notifyError("Receive with RDMA_RECV failed.");
    return false;
  }
  return true;
}



// for RC & UC
bool rdmaRead(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
              uint32_t lkey, uint32_t remoteRKey, bool signal, uint64_t wrID) {
  struct ibv_sge sg;
  struct ibv_send_wr wr;
  struct ibv_send_wr *wrBad;

  fillSgeWr(sg, wr, source, size, lkey);

  wr.opcode = IBV_WR_RDMA_READ;

  if (signal) {
    wr.send_flags = IBV_SEND_SIGNALED;
  }

  wr.wr.rdma.remote_addr = dest;
  wr.wr.rdma.rkey = remoteRKey;
  wr.wr_id = wrID;

  if (ibv_post_send(qp, &wr, &wrBad)) {
    Debug::notifyError("Send with RDMA_READ failed.");
    return false;
  }
  return true;
}


// for RC & UC
bool rdmaWrite(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
               uint32_t lkey, uint32_t remoteRKey, int32_t imm, bool isSignaled,
               uint64_t wrID) {

  struct ibv_sge sg;
  struct ibv_send_wr wr;
  struct ibv_send_wr *wrBad;

  fillSgeWr(sg, wr, source, size, lkey);

  if (imm == -1) {
    wr.opcode = IBV_WR_RDMA_WRITE;
  } else {
    wr.imm_data = imm;
    wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
  }

  if (isSignaled) {
    wr.send_flags = IBV_SEND_SIGNALED;
  }

  wr.wr.rdma.remote_addr = dest;
  wr.wr.rdma.rkey = remoteRKey;
  wr.wr_id = wrID;

  if (ibv_post_send(qp, &wr, &wrBad) != 0) {
    Debug::notifyError("Send with RDMA_WRITE(WITH_IMM) failed.");
    sleep(10);
    return false;
  }
  return true;
}

// RC & UC
bool rdmaFetchAndAdd(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t add,
                     uint32_t lkey, uint32_t remoteRKey) {
  struct ibv_sge sg;
  struct ibv_send_wr wr;
  struct ibv_send_wr *wrBad;

  fillSgeWr(sg, wr, source, 8, lkey);

  wr.opcode = IBV_WR_ATOMIC_FETCH_AND_ADD;
  wr.send_flags = IBV_SEND_SIGNALED;

  wr.wr.atomic.remote_addr = dest;
  wr.wr.atomic.rkey = remoteRKey;
  wr.wr.atomic.compare_add = add;

  if (ibv_post_send(qp, &wr, &wrBad)) {
    Debug::notifyError("Send with ATOMIC_FETCH_AND_ADD failed.");
    return false;
  }
  return true;
}

bool rdmaFetchAndAddBoundary(ibv_qp *qp, uint64_t source, uint64_t dest,
                             uint64_t add, uint32_t lkey, uint32_t remoteRKey,
                             uint64_t boundary, bool singal, uint64_t wr_id) {
  struct ibv_sge sg;
  struct ibv_exp_send_wr wr;
  struct ibv_exp_send_wr *wrBad;

  fillSgeWr(sg, wr, source, 8, lkey);

  wr.exp_opcode = IBV_EXP_WR_EXT_MASKED_ATOMIC_FETCH_AND_ADD;
  wr.exp_send_flags = IBV_EXP_SEND_EXT_ATOMIC_INLINE;
  wr.wr_id = wr_id;

  if (singal) {
    wr.exp_send_flags |= IBV_EXP_SEND_SIGNALED;
  }

  wr.ext_op.masked_atomics.log_arg_sz = 3;
  wr.ext_op.masked_atomics.remote_addr = dest;
  wr.ext_op.masked_atomics.rkey = remoteRKey;

  auto &op = wr.ext_op.masked_atomics.wr_data.inline_data.op.fetch_add;
  op.add_val = add;
  op.field_boundary = 1ull << boundary;

  if (ibv_exp_post_send(qp, &wr, &wrBad)) {
    Debug::notifyError("Send with MASK FETCH_AND_ADD failed.");
    return false;
  }
  return true;
}


// for RC & UC
bool rdmaCompareAndSwap(ibv_qp *qp, uint64_t source, uint64_t dest,
                        uint64_t compare, uint64_t swap, uint32_t lkey,
                        uint32_t remoteRKey, bool signal, uint64_t wrID) {
  struct ibv_sge sg;
  struct ibv_send_wr wr;
  struct ibv_send_wr *wrBad;

  fillSgeWr(sg, wr, source, 8, lkey);

  wr.opcode = IBV_WR_ATOMIC_CMP_AND_SWP;

  if (signal) {
    wr.send_flags = IBV_SEND_SIGNALED;
  }

  wr.wr.atomic.remote_addr = dest;
  wr.wr.atomic.rkey = remoteRKey;
  wr.wr.atomic.compare_add = compare;
  wr.wr.atomic.swap = swap;
  wr.wr_id = wrID;

  if (ibv_post_send(qp, &wr, &wrBad)) {
    Debug::notifyError("Send with ATOMIC_CMP_AND_SWP failed.");
    sleep(5);
    return false;
  }
  return true;
}

bool rdmaCompareAndSwapMask(ibv_qp *qp, uint64_t source, uint64_t dest,
                            uint64_t compare, uint64_t swap, uint32_t lkey,
                            uint32_t remoteRKey, uint64_t mask, bool singal) {
  struct ibv_sge sg;
  struct ibv_exp_send_wr wr;
  struct ibv_exp_send_wr *wrBad;

  fillSgeWr(sg, wr, source, 8, lkey);

  wr.exp_opcode = IBV_EXP_WR_EXT_MASKED_ATOMIC_CMP_AND_SWP;
  wr.exp_send_flags = IBV_EXP_SEND_EXT_ATOMIC_INLINE;

  if (singal) {
    wr.exp_send_flags |= IBV_EXP_SEND_SIGNALED;
  }

  wr.ext_op.masked_atomics.log_arg_sz = 3;
  wr.ext_op.masked_atomics.remote_addr = dest;
  wr.ext_op.masked_atomics.rkey = remoteRKey;

  auto &op = wr.ext_op.masked_atomics.wr_data.inline_data.op.cmp_swap;
  op.compare_val = compare;
  op.swap_val = swap;

  op.compare_mask = mask;
  op.swap_mask = mask;

  if (ibv_exp_post_send(qp, &wr, &wrBad)) {
    Debug::notifyError("Send with MASK ATOMIC_CMP_AND_SWP failed.");
    return false;
  }
  return true;
}


bool rdmaWriteBatch(ibv_qp *qp, RdmaOpRegion *ror, int k, bool isSignaled,
                    uint64_t wrID) {

  struct ibv_sge sg[kOroMax];
  struct ibv_send_wr wr[kOroMax];
  struct ibv_send_wr *wrBad;

  for (int i = 0; i < k; ++i) {
    fillSgeWr(sg[i], wr[i], ror[i].source, ror[i].size, ror[i].lkey);

    wr[i].next = (i == k - 1) ? NULL : &wr[i + 1];

    wr[i].opcode = IBV_WR_RDMA_WRITE;

    if (i == k - 1 && isSignaled) {
      wr[i].send_flags = IBV_SEND_SIGNALED;
    }

    wr[i].wr.rdma.remote_addr = ror[i].dest;
    wr[i].wr.rdma.rkey = ror[i].remoteRKey;
    wr[i].wr_id = wrID;
  }

  if (ibv_post_send(qp, &wr[0], &wrBad) != 0) {
    Debug::notifyError("Send with RDMA_WRITE(WITH_IMM) failed.");
    sleep(10);
    return false;
  }
  return true;
}

bool rdmaCasRead(ibv_qp *qp, const RdmaOpRegion &cas_ror,
                 const RdmaOpRegion &read_ror, uint64_t compare, uint64_t swap,
                 bool isSignaled, uint64_t wrID) {

  struct ibv_sge sg[2];
  struct ibv_send_wr wr[2];
  struct ibv_send_wr *wrBad;

  fillSgeWr(sg[0], wr[0], cas_ror.source, 8, cas_ror.lkey);
  wr[0].opcode = IBV_WR_ATOMIC_CMP_AND_SWP;
  wr[0].wr.atomic.remote_addr = cas_ror.dest;
  wr[0].wr.atomic.rkey = cas_ror.remoteRKey;
  wr[0].wr.atomic.compare_add = compare;
  wr[0].wr.atomic.swap = swap;
  wr[0].next = &wr[1];

  fillSgeWr(sg[1], wr[1], read_ror.source, read_ror.size, read_ror.lkey);
  wr[1].opcode = IBV_WR_RDMA_READ;
  wr[1].wr.rdma.remote_addr = read_ror.dest;
  wr[1].wr.rdma.rkey = read_ror.remoteRKey;
  wr[1].wr_id = wrID;
  wr[1].send_flags |= IBV_SEND_FENCE;
  if (isSignaled) {
    wr[1].send_flags |= IBV_SEND_SIGNALED;
  }

  if (ibv_post_send(qp, &wr[0], &wrBad)) {
    Debug::notifyError("Send with CAS_READs failed.");
    sleep(10);
    return false;
  }
  return true;
}

bool rdmaWriteFaa(ibv_qp *qp, const RdmaOpRegion &write_ror,
                  const RdmaOpRegion &faa_ror, uint64_t add_val,
                  bool isSignaled, uint64_t wrID) {

  struct ibv_sge sg[2];
  struct ibv_send_wr wr[2];
  struct ibv_send_wr *wrBad;

  fillSgeWr(sg[0], wr[0], write_ror.source, write_ror.size, write_ror.lkey);
  wr[0].opcode = IBV_WR_RDMA_WRITE;
  wr[0].wr.rdma.remote_addr = write_ror.dest;
  wr[0].wr.rdma.rkey = write_ror.remoteRKey;
  wr[0].next = &wr[1];

  fillSgeWr(sg[1], wr[1], faa_ror.source, 8, faa_ror.lkey);
  wr[1].opcode = IBV_WR_ATOMIC_FETCH_AND_ADD;
  wr[1].wr.atomic.remote_addr = faa_ror.dest;
  wr[1].wr.atomic.rkey = faa_ror.remoteRKey;
  wr[1].wr.atomic.compare_add = add_val;
  wr[1].wr_id = wrID;

  if (isSignaled) {
    wr[1].send_flags |= IBV_SEND_SIGNALED;
  }

  if (ibv_post_send(qp, &wr[0], &wrBad)) {
    Debug::notifyError("Send with Write Faa failed.");
    sleep(10);
    return false;
  }
  return true;
}

bool rdmaWriteCas(ibv_qp *qp, const RdmaOpRegion &write_ror,
                  const RdmaOpRegion &cas_ror, uint64_t compare, uint64_t swap,
                  bool isSignaled, uint64_t wrID) {

  struct ibv_sge sg[2];
  struct ibv_send_wr wr[2];
  struct ibv_send_wr *wrBad;

  fillSgeWr(sg[0], wr[0], write_ror.source, write_ror.size, write_ror.lkey);
  wr[0].opcode = IBV_WR_RDMA_WRITE;
  wr[0].wr.rdma.remote_addr = write_ror.dest;
  wr[0].wr.rdma.rkey = write_ror.remoteRKey;
  wr[0].next = &wr[1];

  fillSgeWr(sg[1], wr[1], cas_ror.source, 8, cas_ror.lkey);
  wr[1].opcode = IBV_WR_ATOMIC_CMP_AND_SWP;
  wr[1].wr.atomic.remote_addr = cas_ror.dest;
  wr[1].wr.atomic.rkey = cas_ror.remoteRKey;
  wr[1].wr.atomic.compare_add = compare;
  wr[1].wr.atomic.swap = swap;
  wr[1].wr_id = wrID;

  if (isSignaled) {
    wr[1].send_flags |= IBV_SEND_SIGNALED;
  }

  if (ibv_post_send(qp, &wr[0], &wrBad)) {
    Debug::notifyError("Send with Write Cas failed.");
    sleep(10);
    return false;
  }
  return true;
}
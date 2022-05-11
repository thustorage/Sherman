# Sherman: A Write-Optimized Distributed B+Tree Index on Disaggregated Memory 

Sherman is a B+Tree on disaggregated memory; it uses one-sided RDMA verbs to perform all index operations.
Sherman includes three techniques to boost write performance:

- A hierarchical locks leveraging on-chip memory of RDMA NICs.
- Coalescing dependent RDMA commands 
- Two-level version layout in leaf nodes

For more details, please refer to our [paper](https://arxiv.org/abs/2112.07320):

[**SIGMOG'22**] Sherman: A Write-Optimized Distributed B+Tree Index on Disaggregated Memory. Qing Wang and Youyou Lu and Jiwu Shu.


## System Requirements

1. Mellanox ConnectX-5 NICs and above
2. RDMA Driver: MLNX_OFED_LINUX-4.7-3.2.9.0 (If you use MLNX_OFED_LINUX-5**, you should modify codes to resolve interface incompatibility)
3. NIC Firmware: version 16.26.4012 and above (to support on-chip memory, you can use `ibstat` to obtain the version)
4. memcached (to exchange QP information)
5. cityhash
6. boost 1.53 (to support `boost::coroutines::symmetric_coroutine`)


## Getting Started

- `cd Sherman`
- `./script/hugepage.sh` to request huge pages from OS (use `./script/clear_hugepage.sh` to return huge pages)
- `mkdir build; cd build; cmake ..; make -j`
- `cp ../script/restartMemc.sh .`
- configure `../memcached.conf`, where the 1st line is memcached IP, the 2nd is memcached port

For each run:
- `./restartMemc.sh` (to initialize memcached server)
- In each server, execute `./benchmark kNodeCount kReadRatio kThreadCount`

>  We emulate each server as one compute node and one memory node: In each server, as the compute node, 
we launch `kThreadCount` client threads; as the memory node, we launch one memory thread.

> In `./test/benchmark.cpp`, we can modify `kKeySpace` and `zipfan`, to generate different workloads.
> In addition, we can open the macro `USE_CORO` to bind `kCoroCnt` coroutine on each client thread.

## TODO
- Re-write `delete` operations

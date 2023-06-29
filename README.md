# Sherman: A Write-Optimized Distributed B+Tree Index on Disaggregated Memory 

Sherman is a B+Tree on disaggregated memory; it uses one-sided RDMA verbs to perform all index operations.
Sherman includes three techniques to boost write performance:

- A hierarchical locks leveraging on-chip memory of RDMA NICs.
- Coalescing dependent RDMA commands 
- Two-level version layout in leaf nodes

For more details, please refer to our [paper](https://dl.acm.org/doi/abs/10.1145/3514221.3517824):

[**SIGMOG'22**] Sherman: A Write-Optimized Distributed B+Tree Index on Disaggregated Memory. Qing Wang and Youyou Lu and Jiwu Shu.


## System Requirements

1. Mellanox ConnectX-5 NICs and above
2. RDMA Driver: MLNX_OFED_LINUX-4.7-3.2.9.0 (If you use MLNX_OFED_LINUX-5**, you should modify codes to resolve interface incompatibility)
3. NIC Firmware: version 16.26.4012 and above (to support on-chip memory, you can use `ibstat` to obtain the version)
4. memcached (to exchange QP information)
5. cityhash
6. boost 1.53 (to support `boost::coroutines::symmetric_coroutine`)

## Setup about RDMA Network

**1. RDMA NIC Selection.** 

You can modify this line according the RDMA NIC you want to use, where `ibv_get_device_name(deviceList[i])` is the name of RNIC (e.g., mlx5_0)
https://github.com/thustorage/Sherman/blob/9bb950887cd066ebf4f906edbb43bae8e728548d/src/rdma/Resource.cpp#L28

**2. Gid Selection.** 

If you use RoCE, modify `gidIndex` in this line according to the shell command `show_gids`, which is usually 3.
https://github.com/thustorage/Sherman/blob/c5ee9d85e090006df39c0afe025c8f54756a7aea/include/Rdma.h#L60

**3. MTU Selection.** 

If you use RoCE and the MTU of your NIC is not equal to 4200 (check with `ifconfig`), modify the value `path_mtu` in `src/rdma/StateTrans.cpp`

**4. On-Chip Memory Size Selection.** 

Change the constant ``kLockChipMemSize`` in `include/Commmon.h`, making it <= max size of on-chip memory.

## Getting Started

- `cd Sherman`
- `./script/hugepage.sh` to request huge pages from OS (use `./script/clear_hugepage.sh` to return huge pages)
- `mkdir build; cd build; cmake ..; make -j`
- `cp ../script/restartMemc.sh .`
- configure `../memcached.conf`, where the 1st line is memcached IP, the 2nd is memcached port

For each run with `kNodeCount` servers:
- `./restartMemc.sh` (to initialize memcached server)
- In each server, execute `./benchmark kNodeCount kReadRatio kThreadCount`

>  We emulate each server as one compute node and one memory node: In each server, as the compute node, 
we launch `kThreadCount` client threads; as the memory node, we launch one memory thread. `kReadRatio` is the ratio of `get` operations.

> In `./test/benchmark.cpp`, we can modify `kKeySpace` and `zipfan`, to generate different workloads.
> In addition, we can open the macro `USE_CORO` to bind `kCoroCnt` coroutine on each client thread.

## Known bugs

- The two-level version may induce inconsistency in some concurrent cases. Refer to [this SIGMOD'23 paper](https://dl.acm.org/doi/10.1145/3589276)

## TODO
- Re-write `delete` operations

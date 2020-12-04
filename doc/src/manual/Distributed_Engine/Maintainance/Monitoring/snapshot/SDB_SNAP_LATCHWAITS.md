##描述##

闩锁等待快照 SDB_SNAP_LATCHWAITS 列出数据库中正在发生的闩锁等待信息。

也可以通过"viewHistory"快照选项查看历史闩锁等待信息, 当等待的线程拿到该闩锁时，该次等待记录会被归入历史闩锁等待信息。

每一个数据节点上正在进行的每一个闩锁等待为一条记录。

##标示##

SDB_SNAP_LATCHWAITS

###字段信息###

| 字段名                 | 类型     | 描述                                                         |
| ---------------------- | -------- | ------------------------------------------------------------ |
| NodeName               | 字符串   | 闩锁等待发生的所在节点名                                     |
| WaiterTID              | 整型     | 等待闩锁的线程ID                                             |
| RequiredMode           | 字符串   | 上述线程要求获得的闩锁模式, 分为S共享模式和X排他模式两种     |
| LatchName              | 字符串   | 被等待闩锁对象的名称                                         |
| Address                | 字符串   | 被等待闩锁对象的地址                                         |
| StartTimestamp         | 字符串   | 本次等待开始时间                                             |
| LatchWaitTime          | 整型     | 本次等待耗费时间, 单位：毫秒                                 |
| LatestOwner            | 整型     | 最近获得该闩锁的线程ID                                       |
| LatestOwnerMode        | 字符串   | 最近获得该闩锁线程所获得的模式，分为S共享模式和X排他模式两种 |
| NumOwner               | 整型     | 该等待事件发生时被等待闩锁总共的持有者数量                   |

##示例##

即时闩锁等待信息示例。

```
> db.snapshot(SDB_SNAP_LATCHWAITS)
{
  "NodeName": "yang-VirtualBox:11870",
  "WaiterTID": 24118,
  "RequiredMode": "S",
  "LatchName": "catGTSMsgHandler jobLatch",
  "Address": "0x7fd3740ae410",
  "StartTimestamp": "2020-06-13-02.52.50.336383",
  "LatchWaitTime": 34.806,
  "LatestOwner": 24109,
  "LatestOwnerMode": "X",
  "NumOwner": 1
}
...
...
>
```
以上述输出为例\
现在系统中正在发生一个闩锁等待事件\
线程24118正在等待获取"catGTSMsgHandler jobLatch"闩锁的S共享模式。\
到目前为止已经等待了34.806毫秒。\
线程24109是最近一个拿到这个闩锁的线程。拿到了X排他模式。此时闩锁只有24109一个拥有者。

历史闩锁等待信息示例

```
> db.snapshot(SDB_SNAP_LATCHWAITS, new SdbSnapshotOption().options({"viewHistory":true}))
...
...
{
  "NodeName": "yang-VirtualBox:11870",
  "WaiterTID": 9726,
  "RequiredMode": "X",
  "LatchName": "dpsTransLockManager rwMutex",
  "Address": "0x7f8dca267d40",
  "StartTimestamp": "2020-06-12-04.02.22.096686",
  "LatchWaitTime": 1.172,
  "LatestOwner": 13608,
  "LatestOwnerMode": "S",
  "NumOwner": 1
}
...
...
>
```

以上述输出为例\
系统中之前发生了一次等待事件。\
线程9726等待获得"dpsTransLockManager rwMutex"闩锁X排他模式。\
从"2020-06-12-04.02.22.096686"开始等待，在获得闩锁前总共等了1.172毫秒。\
在它之前获得这个闩锁的是线程13608，获得了S共享模式闩锁。当时只有13608一个拥有者。


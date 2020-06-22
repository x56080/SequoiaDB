##描述##

闩锁等待快照 $SNAPSHOT_LATCHWAITS 列出数据库中正在进行的闩锁等待信息。

每一个数据节点上正在进行的每一个闩锁等待为一条记录。

##标示##

$SNAPSHOT_LATCHWAITS

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


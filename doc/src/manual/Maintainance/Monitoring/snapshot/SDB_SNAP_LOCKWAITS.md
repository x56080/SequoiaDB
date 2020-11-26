##描述##

锁等待快照 SDB_SNAP_LOCKWAITS 列出数据库中正在发生的锁等待信息。

也可以通过"viewHistory"快照选项查看历史锁等待信息, 当等待的线程拿到该锁时，该次等待记录会被归入历史锁等待信息。

每一个数据节点上正在进行的每一个锁等待为一条记录。

##标示##

SDB_SNAPSHOT_LOCKWAITS

###字段信息###

| 字段名                 | 类型     | 描述                                                           |
| ---------------------- | -------- | -------------------------------------------------------------- |
| NodeName               | 字符串   | 锁等待发生的所在节点名                                         |
| WaiterTID              | 整型     | 等待锁的线程ID                                                 |
| RequiredMode           | 字符串   | 上述线程要求获得的锁模式, 分为S共享模式和X排他模式两种         |
| CSID                   | 整型     | 被等待锁对象所在集合空间的 ID                                  |
| CLID                   | 整型     | 被等待锁对象所在集合的 ID                                      |
| ExtentID               | 整型     | 被等待锁对象所在记录的 ID                                      |
| Offset                 | 整型     | 被等待锁对象所在记录的偏移量                                   |
| StartTimestamp         | 字符串   | 本次等待开始时间                                               |
| TransLockWaitTime      | 整型     | 锁等待耗费时间，单位：毫秒                                     |
| LatestOwner            | 整型     | 最近获得该锁的线程ID                                           |
| LatestOwnerMode        | 字符串   | 最近获得该锁的线程所获得的模式，分为S共享模式和X排他模式两种   |
| NumOwner               | 整型     | 该等待事件发生时被等待闩锁总共的持有者数量                     |

##示例##

即时锁等待信息示例。

```
> db.snapshot(SDB_SNAP_LOCKWAITS)
{
  "NodeName": "yang-VirtualBox:11870",
  "WaiterTID": 23853,
  "RequiredMode": "X",
  "CSID": 4,
  "CLID": 7,
  "ExtentID": 838,
  "Offset": 53396,
  "StartTimestamp": "2020-06-13-02.52.38.470191",
  "TransLockWaitTime": 18.815,
  "LatestOwner": 23532,
  "LatestOwnerMode": "X",
  "NumOwner": 1
}
...
...
>
```

历史锁等待信息示例。

```
> db.snapshot(SDB_SNAP_LOCKWAITS, new SdbSnapshotOption().options({"viewHistory":true}))
{
  "NodeName": "yang-VirtualBox:11870",
  "WaiterTID": 13602,
  "RequiredMode": "X",
  "CSID": 3,
  "CLID": 7,
  "ExtentID": 483,
  "Offset": 57688,
  "StartTimestamp": "2020-06-12-04.04.01.300151",
  "TransLockWaitTime": 14.05,
  "LatestOwner": 10307,
  "LatestOwnerMode": "X",
  "NumOwner": 1
}
{
  "NodeName": "yang-VirtualBox:11870",
  "WaiterTID": 13603,
  "RequiredMode": "X",
  "CSID": 3,
  "CLID": 8,
  "ExtentID": 486,
  "Offset": 48884,
  "StartTimestamp": "2020-06-12-04.04.01.173701",
  "TransLockWaitTime": 14.94,
  "LatestOwner": 13635,
  "LatestOwnerMode": "X",
  "NumOwner": 1
}
{
  "NodeName": "yang-VirtualBox:11870",
  "WaiterTID": 10398,
  "RequiredMode": "X",
  "CSID": 3,
  "CLID": 7,
  "ExtentID": 483,
  "Offset": 45476,
  "StartTimestamp": "2020-06-12-04.04.01.018260",
  "TransLockWaitTime": 83.337,
  "LatestOwner": 12051,
  "LatestOwnerMode": "X",
  "NumOwner": 1
}
...
...
>
```

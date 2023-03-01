[^_^]: 

    数据库快照
    作者：何嘉文
    时间：20190307
    评审意见

    王涛：
    许建辉：
    市场部：20190425


当前事务快照可以列出当前会话正在进行的事务信息，当前会话在每一个数据节点上正在进行的事务为一条记录。

> **Note:**
>
> 一般每个会话在每个数据节点上只有一个事务记录。

##标识##

SDB_SNAP_TRANSACTIONS_CURRENT


##字段信息##

| 字段名                 | 类型     | 描述                                     |
| ---------------------- | -------- | ---------------------------------------- |
| NodeName               | string   | 节点名，格式为<主机名>:<服务名>          |
| SessionID              | int64    | 会话 ID                                  |
| TransactionID          | string   | 事务 ID                                  |
| TransactionIDSN        | int64    | 事务序列号                               |
| IsRollback             | boolean  | 事务是否处于回滚中                       |
| CurrentTransLSN        | int64    | 事务最后一条记录对应的 LSN<br>该字段可用于检查是否存在空闲事务，具体可参考[查询空闲事务][residualtransaction] |
| BeginTransLSN          | int64    | 事务第一条记录对应的 LSN<br>该字段可用于查询最早开启的事务，当日志空间不足时，可以提交最早的事务以释放日志空间 |
| WaitLock               | bson     | 正在等待的锁                             |
| TransactionLocksNum    | int32    | 事务已经获得的锁                         |
| RelatedID              | string   | 内部标识                                 |
| GotLocks               | bson array| 事务已经获得的锁列表                    |

> **Note:**  
>
> 当 WaitLock 没有锁对象时，表示事务没有在等待锁。

###锁对象信息###

WaitLock 和 GetLocks 字段中锁对象的信息如下：

| 字段名       | 类型   | 描述                     |
| ------------ | ------ | ------------------------ |
| CSID         | int32  | 锁对象所在集合空间的 ID  |
| CLID         | int32  | 锁对象所在集合的 ID      |
| ExtentID     | int32  | 锁对象所在记录的 ID      |
| Offset       | int32  | 锁对象所在记录的偏移量   |
| Mode         | string | 锁的类型，取值如下：<br>IS：意向共享锁<br>IX：意向排他锁<br>S：共享锁<br>U：升级锁<br>X：排他锁 |
| Count        | int32  | 锁计数器（只在 GetLocks 中存在） |
| Duration     | int64  | 锁的持有或等待时间，单位为毫秒   |

###锁对象的描述###

锁对象每个字段取值不同表示不同的锁对象

| 锁对象       | CSID | CLID  | ExtentID | Offset | 备注 |
| ------------ | ---- | ----- | ---- | ---- | ------------ |
| 没有锁对象   | -1   | 65535 | -1   | -1   | 一般在WaitLock为没有锁对象时，表示当前事务没有在等待锁 |
| 集合空间锁   | >= 0 | 65535 | -1   | -1   | |
| 集合锁       | >= 0 | >= 0  | -1   | -1   | |
| 记录锁       | >= 0 | >= 0  | >= 0 | >= 0 | |

##应用场景##

###查看快照信息###

查看当前事务快照

```lang-javascript
> db.snapshot(SDB_SNAP_TRANSACTIONS_CURRENT)
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver:11830",
  "SessionID": 89,
  "TransactionID": "03e80000000001",
  "IsRollback": false,
  "CurrentTransLSN": -1,
  "WaitLock": {},
  "TransactionLocksNum": 3,
  "RelatedID": "c0a81457c3500000000000000059",
  "GotLocks": [
    {
      "CSID": 1,
      "CLID": 0,
      "ExtentID": 9,
      "Offset": 36,
      "Mode": "U",
      "Count": 1,
      "Duration": 1137053
    },
    {
      "CSID": 1,
      "CLID": 0,
      "ExtentID": -1,
      "Offset": -1,
      "Mode": "IS",
      "Count": 1,
      "Duration": 1137053
    },
    {
      "CSID": 1,
      "CLID": 65535,
      "ExtentID": -1,
      "Offset": -1,
      "Mode": "IS",
      "Count": 1,
      "Duration": 1137053
    }
  ]
}
```

###查询空闲事务###

用户可通过对比当前事务快照下字段 CurrentTransLSN 与[节点健康检测快照][SDB_SNAP_HEALTH]下字段 CurrentLSN 的值，检查当前会话是否存在长时间运行且未写入数据的空闲事务。如果取值相差过大，说明该事务为空闲事务。该类事务将占用大量数据库资源，同时导致日志空间不足，建议及时提交。具体操作步骤如下：

1. 通过事务快照查看字段 CurrentTransLSN

    ```lang-javascript
    > db.snapshot(SDB_SNAP_TRANSACTIONS_CURRENT, {}, {NodeName: null, TransactionID: null, CurrentTransLSN: null})
    ```

    输出结果如下：

    ```lang-json
    ...
    {
      "NodeName": "sdbserver:11820",
      "TransactionID": "0x00020067a74f69",
      "CurrentTransLSN": 83624
    }
    ...
    ```

2. 通过节点健康检测快照查看字段 CurrentLSN

    ```lang-javascript
    > db.snapshot(SDB_SNAP_TRANSACTIONS_CURRENT, {}, {NodeName: null, CurrentLSN: null})
    ```

    输出结果如下：

    ```lang-json
    ...
    {
      "NodeName": "sdbserver:11820",
      "CurrentLSN": {
        "Offset": 157288368,
        "Version": 1
      }
    }
    ...
    ```

[^_^]:
    本文使用的所有引用及链接
[residualtransaction]:manual/Manual/Snapshot/SDB_SNAP_TRANSACTIONS_CURRENT.md#查询空闲事务
[SDB_SNAP_HEALTH]:manual/Manual/Snapshot/SDB_SNAP_HEALTH.md
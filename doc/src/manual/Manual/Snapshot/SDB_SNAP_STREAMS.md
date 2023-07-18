[^_^]:
    数据流快照

数据流快照可以列出当前集群中所有正在运行的数据流的详细信息。

##标识##

SDB_SNAP_STREAMS

##字段信息##

| 字段名 | 类型 | 描述 |
| ---- | ---- | ---- |
| NodeName | string | 所在的节点名 |
| GroupName | string | 所在的复制组名 |
| Type | string | 数据流类型，取值如下：<br>"change"：变更流 |
| SessionID | int64 | 执行数据流的会话的唯一标识 |
| ContextID | int64 | 执行数据流的上下文的唯一标识 |
| Options | bson | 数据流的配置参数 |
| CurToken | string | 数据流的当前位置 |
| CurTokenDesc | bson | 数据流的当前位置的描述 |
| CurTokenDesc.Version | int32 | 数据流的当前位置的版本号 |
| CurTokenDesc.Type | string | 数据流的当前位置的类型，取值如下：<br>"change"：变更流的当前位置 |
| CurTokenDesc.TokenFlags | int32 | 数据流的当前位置的标记 |
| CurTokenDesc.Source | int32 | 数据流的当前位置的来源（复制组唯一标识） |
| CurTokenDesc.GlobalTimestamp | int64 | 数据流的当前位置的全局逻辑时间（当数据流的当前位置的类型为 "change" 时展示） |
| CurTokenDesc.LSN | int64 | 数据流的当前位置对应的LSN偏移（当数据流的当前位置的类型为 "change" 时展示） |
| CurTokenDesc.CheckCode | int32 | 数据流的当前位置的校验码（对应的LSN版本号，当数据流的当前位置的类型为 "change" 时展示） |
| StartTimestamp | string | 数据流开始的时间 |
| TimeSpent | double | 数据流已执行时间，单位：秒 |
| ControlNum | int64 | 数据流返回的控制记录的总数量 |
| ChangeNum | int64 | 数据流返回的变更记录的总数量 |
| DataNum | int64 | 数据流返回的数据记录的总数量 |
| BatchNum | int64 | 数据流返回的批次的总数量 |
| ReturnNum | int64 | 数据流返回的记录的总数量 |
| ReturnSize | int64 | 数据流返回的记录的总大小，单位：字节 |
| Speed | double | 数据流的读取速度，单位：MB/秒 |
| SourceStats | bson | 数据流的来源 |
| SourceStats.ReceivedNum | int64 | 数据流的来源的接收的记录的总数量 |
| SourceStats.WaitTime | double | 数据流的来源的等待时间，单位：秒 |
| SourceStats.HitCacheNum | int64 | 数据流的来源的命中缓存记录的总数量 |
| SourceStats.MissCacheNum | int64 | 数据流的来源的未命中缓存记录的总数量 |
| SourceStats.ScannedNum | int64 | 数据流的来源的扫描记录的总数量 |
| SourceStats.ScannedSize | int64 | 数据流的来源的扫描记录的总大小，单位：字节 |
| SourceStats.QueueCapacity | int64 | 数据流的来源的队列容量 |
| SourceStats.InQueueNum | int64 | 数据流的来源的队列中当前的记录的总数量 |
| SourceStats.InQueueSize | int64 | 数据流的来源的队列中当前的记录的总大小，单位：字节 |
| SourceStats.CacheCapacity | int64 | 数据流的来源的缓存容量 |
| SourceStats.InCacheNum | int64 | 数据流的来源的缓存中当前的记录的总数量 |
| SourceStats.InCacheSize | int64 | 数据流的来源的缓存中当前的记录的总大小，单位：字节 |

##示例##

查看数据流快照

```lang-javascript
> db.snapshot(SDB_SNAP_STREAMS)
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver:20000",
  "GroupName": "group1",
  "Type": "change",
  "SessionID": 26,
  "ContextID": 2,
  "Options": {
    "Token": "",
    "Collections": [
      "foo.bar"
    ],
    "CollectionSpaces": []
  },
  "CurToken": "00010000000003e80000000000000000000000000000013c0000000100000000",
  "CurTokenDesc": {
    "Version": 0,
    "Type": "change",
    "TokenFlags": 0,
    "Source": 1000,
    "GlobalTimestamp": 0,
    "LSN": 316,
    "CheckCode": 1
  },
  "StartTimestamp": "2023-07-15-20.22.26.107000",
  "TimeSpent": 86.513,
  "ControlNum": 85,
  "ChangeNum": 1,
  "DataNum": 0,
  "BatchNum": 86,
  "ReturnNum": 86,
  "ReturnSize": 5636096,
  "Speed": 0.06212939095858425,
  "SourceStats": {
    "ReceivedNum": 1,
    "WaitTime": 85.9,
    "HitCacheNum": 1,
    "MissCacheNum": 0,
    "ScannedNum": 0,
    "ScannedSize": 0,
    "QueueCapacity": 167772160,
    "InQueueNum": 0,
    "InQueueSize": 0,
    "CacheCapacity": 33554432,
    "InCacheNum": 0,
    "InCacheSize": 0
  }
}
```


[^_^]:
    本文使用的所有引用及链接
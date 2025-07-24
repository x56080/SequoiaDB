##名称##

getSyncClients - 获取 STP 节点所在集群的时间同步信息

##语法##

**stp.getSyncClients()**

##类别##

Stp

##描述##

该函数用于获取 STP 节点所在集群的同步间隔、时间容错误差等时间同步信息。

##参数##

无

##返回值##

函数执行成功时，将通过游标（SdbCursor）返回 STP 节点所在集群的时间同步信息列表，返回的字段信息可参考 [stpq 查询同步客户端信息][stpq]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0 及以上版本

##示例##

获取 STP 节点所在集群的时间同步信息

```lang-javascript
> var stp = new Stp()
> stp.getSyncClients()
{
  "SyncSource": {
    "HostName": "server-1",
    "Service": "9622"
  },
  "SyncClients": [
    {
      "Role": "client",
      "HostName": "server-2",
      "Service": "9622",
      "OID": {
        "$oid": "6018fc765f862e2e3241e692"
      },
      "SyncInterval": 60,
      "TimeError": 5559908,
      "MaxTimeError": 50000000,
      "SyncPassed": 470,
      "SyncCount": 47,
      "SyncStatus": "CheckOffset",
      "SyncPort": 9622
    }
  ]
}
```

[^_^]:
    本文使用的所有引用及链接
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

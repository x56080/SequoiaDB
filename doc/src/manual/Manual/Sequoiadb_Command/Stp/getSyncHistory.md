##名称##

getSyncHistory - 获取 STP 节点与各同步源的历史时间同步信息

##语法##

**stp.getSyncHistory()**

##类别##

Stp

##描述##

该函数用于获取 STP 节点与各同步源的同步次数、时间偏移等历史时间同步信息。

##参数##

无

##返回值##

函数执行成功时，将通过游标（SdbCursor）返回 STP 节点与各同步源的历史时间同步信息列表，返回的字段信息可参考 [stpq 查询同步历史][stpq]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0 及以上版本

##示例##

获取 STP 节点与各同步源的历史时间同步信息

```lang-javascript
> var stp = new Stp()
> stp.getSyncHistory()
{
  "SyncSources": [
    {
      "Role": "server",
      "HostName": "server-1",
      "Service": "9622",
      "SyncCount": 116,
      "ValidCount": 89,
      "MinDelay": 250910,
      "MaxDelay": 19122284,
      "InitOffset": 98988348233399,
      "NegOffset": {
        "Count": 46,
        "Min": -9898,
        "Max": -5513457
      },
      "PosOffset": {
        "Count": 42,
        "Min": 3020,
        "Max": 4935051
      },
      "LastDelay": 584499,
      "LastOffset": -20017,
      "LastPassed": 110
    },
    ...
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

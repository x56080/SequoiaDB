##名称##

getServers - 获取 STP 节点所同步的 server 组信息

##语法##

**stp.getServers()**

##类别##

Stp

##描述##

该函数用于获取 STP 节点所同步的 server 组信息，STP 节点与 server 组的说明可参考[逻辑时间][logicaltime]。

##参数##

无

##返回值##

函数执行成功时，将通过游标（SdbCursor）返回 STP 节点所同步的 server 组信息列表，返回的字段信息可参考 [stpq 查询 server 信息][stpq]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0 及以上版本

##示例##

获取 STP 节点所同步的 server 组信息

```lang-javascript
> var stp = new Stp()
> stp.getServers()
{
  "Version": 1,
  "Group": [
    {
      "Role": "server",
      "HostName": "server-1",
      "Service": "9622"
    }
  ],
  "PrimaryNode": {
    "HostName": "server-1",
    "Service": "9622"
  }
}
```

[^_^]:
    本文使用的所有引用及链接
[logicaltime]:manual/Distributed_Engine/Architecture/Stp/logicaltime.md
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

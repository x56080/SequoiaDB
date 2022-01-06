##名称##

getMeta - 获取 STP 节点的元数据信息  

##语法##

**stp.getMeta()**

##类别##

Stp

##描述##

该函数用于获取 STP 节点的版本号、同步间隔等元数据信息。

##参数##

无

##返回值##

函数执行成功时，将通过游标（SdbCursor）返回 STP 节点的元数据信息列表，返回的字段信息可参考 [stpq 查询元数据][stpq]。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0 及以上版本

##示例##

获取 STP 节点的元数据信息

```lang-javascript
> var stp = new Stp()
> stp.getMeta()
{
  "MetaSHMKey": "9622",
  "MetaData": {
    "Version": 1,
    "SyncInterval": 60,
    "SyncHWTime": {
      "Second": 23667387,
      "NanoSecond": 823851456
    },
    "BaseHWTime": {
      "Second": 23664627,
      "NanoSecond": 618702083
    },
    "BaseRealTime": {
      "Second": 1612364947,
      "NanoSecond": 487145000
    },
    "Offset": 0,
    "SlewRate": 10000,
    "TimeError": 1000000
  },
  "MetaLSN": {
    "Offset": 1612367707692292,
    "Version": 2
  }
}
```

[^_^]:
    本文使用的所有引用及链接
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

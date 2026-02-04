##名称##

getDetail - 获取数据中心详细信息

##语法##

**SdbDC.getDetail()**

##类别##

SdbDC

##描述##

该函数用于获取数据中心的详细信息，包括数据中心的状态、配置和统计信息。

##参数##

无

##返回值##

函数执行成功时，返回一个包含数据中心详细信息的 BSON 对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`getDetail()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数类型错误 | 检查参数类型是否正确 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.0 及以上版本

##示例##

**示例1：** 获取数据中心详细信息

```lang-javascript
> var dc = db.getDC()
> dc.getDetail()
{
  "Activated": true,
  "CATVersion": 3,
  "CSUniqueHWM": 258690,
  "DataCenter": {
    "Address": "sdbserver1:11800,sdbserver2:11800,sdbserver3:11800",
    "BusinessName": "yyy",
    "ClusterName": "xxx"
  },
  "GlobalID": 98807,
  "Readonly": false,
  "RecycleBin": {
    "AutoDrop": true,
    "Enable": true,
    "ExpireTime": 4320,
    "MaxItemNum": 30,
    "MaxVersionNum": 2,
    "RecycleIDHWM": 410998
  },
  "TaskHWM": 704332,
  "Type": "GLOBAL",
  "_id": {
    "$oid": "654c6a81030f266b4709758e"
  }
}
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

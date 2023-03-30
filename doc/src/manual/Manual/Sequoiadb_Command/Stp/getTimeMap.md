##名称##

getTimeMap - 获取逻辑时间和对应的系统时间

##语法##

**stp.getTimeMap()**

##类别##

Stp

##描述##

该函数用于获取 STP 节点的逻辑时间和系统时间。

##参数##

无

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取逻辑时间和对应的系统时间，字段说明如下：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| TimeMap.LogicalTime | number | 逻辑时间，单位为微秒 |
| TimeMap.RealTime | timestamp | 系统时间，单位为微秒 |

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0.4 及以上版本

##示例##

获取 STP 节点的逻辑时间和系统时间

```lang-javascript
> var stp = new Stp()
> stp.getTimeMap()
{
  "TimeMap": [
    {
      "LogicalTime": 1679767981830566,
      "RealTime": {
        "$timestamp": "2023-03-24-04.53.47.000610"
      }
    },
    {
      "LogicalTime": 1679707861320036,
      "RealTime": {
        "$timestamp": "2023-03-24-04.51.01.000039"
      }
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
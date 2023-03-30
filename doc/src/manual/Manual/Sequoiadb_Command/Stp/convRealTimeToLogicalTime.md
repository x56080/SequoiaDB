##名称##

convRealTimeToLogicalTime - 将指定的系统时间转换为逻辑时间

##语法##

**stp.convRealTimeToLogicalTime(\<realTime\>)**

##类别##

Stp

##描述##

该函数用于将指定的系统时间转换为逻辑时间。

##参数##

realTime（ *timestamp，必填* ）

系统时间，单位为微秒，格式为 `{$timestamp: "2023-03-20-15.46.16.000582"}`

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取转换后的逻辑时间，字段说明如下：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| LogicalTime | number | 逻辑时间，单位为微秒 |

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0.4 及以上版本

##示例##

将指定的系统时间转换为逻辑时间

```lang-javascript
> var stp = new Stp()
> stp.convRealTimeToLogicalTime({$timestamp: "2023-03-20-15.46.16.000582"})
{
  "LogicalTime": 1679358376000577
}
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
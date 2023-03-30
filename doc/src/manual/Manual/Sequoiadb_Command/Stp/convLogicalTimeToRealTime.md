##名称##

convLogicalTimeToRealTime - 将指定的逻辑时间转换为系统时间

##语法##

**stp.convLogicalTimeToRealTime(\<logicalTime\>)**

##类别##

Stp

##描述##

该函数用于将指定的逻辑时间转换为系统时间。

##参数##

logicalTime（ *number，必填* ）

逻辑时间，单位为微秒

##返回值##

函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取转换后的系统时间，字段说明如下：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| RealTime | timestamp | 系统时间，单位为微秒 |

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0.4 及以上版本

##示例##

1. 获取当前的逻辑时间

    ```lang-javascript
    > var stp = new Stp()
    > stp.getTimeUS()
    {
      "TimeStamp": 1679339776883577,
      "TimeError": 1000000
    }
    ```

2. 将逻辑时间转换为系统时间

    ```lang-javascript
    > stp.convLogicalTimeToRealTime(1679339776883577)
    {
      "RealTime": {
        "$timestamp": "2023-03-20-10.36.16.000582"
      }
    }
    ```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
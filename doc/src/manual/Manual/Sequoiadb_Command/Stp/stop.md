## 名称

stop - 停止 STP 服务进程

## 语法

**stp.stop()**

## 类别

Stp

## 描述

该函数用于停止当前 stp 对象所连接的 [STP 服务][stp]进程。

## 参数

无

## 返回值

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

## 错误

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

## 版本

v5.0 及以上版本

## 示例

停止本地的 STP 服务进程

```lang-javascript
> var stp = new Stp()
> stp.stop()
```

[^_^]:
    本文使用的所有引用及链接
[stp]:manual/Distributed_Engine/Architecture/Stp/Readme.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

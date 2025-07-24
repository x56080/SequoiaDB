## 名称

reelect - 在 STP 节点所在的 server 组中重新选主

## 语法

**stp.reelect([options])**

## 类别

Stp

## 描述

该函数用于在当前 STP 节点所在的 server 组中重新选举主节点。server 组内存活节点数需超过总节点数的 1/2，才能进行选举。

## 参数

options（ *object，选填* ）

通过 options 参数可以设置其他选填参数：

- Seconds（number）：指定选举超时时间，选举将在指定时间内完成，单位为秒，默认值为 30

    该参数取值需大于等于 10s，否则执行相关语句将报错。

    格式：`Seconds:60`

- HostName（string）：指定期望当选主节点的主机名

    格式：`HostName:"sdbserver"`


## 返回值

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

## 错误

`reelect()` 函数常见异常如下：

| 错误码 | 错误类型    | 可能发生的原因         | 解决办法 |
| ------ | --------    | --------------         | --------  |
| -13    | SDB_TIMEOUT | 选举未在指定时间内完成 | -        |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

## 版本

v5.0 及以上版本

## 示例

在当前 STP 节点所在的 server 组中重新选举，并指定选举超时时间为 60s

```lang-javascript
> var stp = new Stp()
> stp.reelect({Seconds: 60})
```

[^_^]:
    本文使用的所有引用和链接
[getServers]:manual/Manual/Sequoiadb_Command/Stp/getServers.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

## 名称

Stp - STP 服务进程对象

## 语法

**var stp = new Stp([hostname],[svcname])**

## 类别

Stp

## 描述

该函数用于新建一个 [STP 服务][stp]进程对象，以连接 STP 节点。

## 参数

* hostname ( *string，选填* )

   目标 STP 所在主机的主机名

* svcname ( *number/string，选填* )

   目标 STP 所使用的端口号， 默认端口号为 9622

## 返回值

函数执行成功时，将返回一个 Stp 对象。

函数执行失败时，将抛异常并输出错误信息。

## 错误

`Stp()` 函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | --- | ------------ | ----------- |
| -15 | SDB_NETWORK | 网络错误 | 检查填写的地址或者端口是否可达。|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

## 版本

v5.0 及以上版本

## 示例

- 连接本地 STP 服务进程对象

    ```lang-javascript
    > var stp = new Stp()
    ```

- 连接指定机器的 STP 服务进程对象

    ```lang-javascript
    > var stp = new Stp("sdbserver", 9622)
    ```


[^_^]:
    本文使用的所有引用及链接
[stp]:manual/Distributed_Engine/Architecture/Stp/Readme.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

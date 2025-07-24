##名称##

removeStp - 删除 STP 服务进程

##语法##

**oma.removeStp()**

##类别##

Oma

##描述##

该函数用于在目标集群控制器（sdbcm）所在的机器中删除 [STP 服务][stp]进程。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`removeStp()` 函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -146 | SDBCM_NODE_NOTEXISTED | STP 节点不存在 | 在 STP 配置文件目录 `<INSTALL_DIR>/conf/stp/stp.conf` 查看 STP 是否存在 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v5.0 及以上版本

##示例##

在本地删除 STP 服务进程

```lang-javascript
> var oma = new Oma("localhost",11790)
> oma.removeStp()
```

[^_^]:
    本文使用的所有引用及链接
[stp]:manual/Distributed_Engine/Architecture/Stp/Readme.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

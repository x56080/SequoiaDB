##名称##

initSecurityKeys - 初始化密钥

##语法##

**db.initSecurityKeys()**

##类别##

Sdb

##描述##

该函数用于初始化密钥，初始化成功后将生成 MK 密钥和 DEK 密钥，分别存储于目录 `<catalog_dbpath>/security/MK` 和 `<catalog_dbpath>/security/DEK` 下，文件说明可参考[密钥文件][data_encryption]。

>**Note:**
>
> 目前仅支持通过本地连接执行该函数。

##参数## 

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`initSecurityKeys()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决方法 |
| ------ | -------- | -------------- | -------- |
| -394   | SDB_OPERATION_DENIED | 非本地连接 | 通过本地连接执行初始化 |
| -398   | SDB_SEC_KEYS_INITIALIZED | 密钥已初始化 | - |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

初始化密钥

```lang-javascript
> db = new Sdb("localhost", 11810)
> db.initSecurityKeys()
```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[data_encryption]:manual/Distributed_Engine/Architecture/data_encryption.md#密钥文件
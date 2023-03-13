##名称##

addSchema - 绑定 Schema

##语法##

**db.collectionspace.collection.addSchema(\<schemaName\>)**

##类别##

SdbInfoSchema

##描述##

该函数用于将集合与 Schema 进行绑定。

##参数##

schemaName（ *string，必填* ）

Schema 的名称   

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`addSchema` 函数常见异常如下：

| 错误码 | 错误类型                | 可能发生的原因 | 解决办法                   |
| ------ | ----------------------- | -------------- | -------------------------- |
| -396   | SDB_SCHEMA_NOT_EXIST    | Schema 不存在  | 检查指定的 Schema 是否存在 |
| -6     | SDB_INVALIDARG          | 参数错误       | 检查参数类型是否填写正确   |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

将集合 sample.employee 与名为“s1”的 Schema 进行绑定

```lang-javascript
> db.sample.employee.addSchema("s1")
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
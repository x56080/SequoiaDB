[^_^]:
     renameColumn

##名称##

renameColumn - 重命名 Schema 中的字段

##语法##

**schema.renameColumn(\<oldName\>, \<newName\>)**

##类别##

SdbInfoSchema

##描述##

该函数用于在 Schema 中对指定的字段进行重命名。

##参数##

- oldName（ *string，必填* ）

     原字段名称

- newName（ *string，必填* ）

     新字段名称    

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

1. 获取指定 Schema 的引用

    ```lang-javascript
    > var schema = db.getSchema("s1")
    ```

2. 将该 Schema 中字段 id 的名称修改为 oid

    ```lang-javascript
    > schema.renameColumn("id", "oid")
    ```




[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
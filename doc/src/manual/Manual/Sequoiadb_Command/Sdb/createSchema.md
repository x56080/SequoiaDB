[^_^]:
     createSchema

##名称##

createSchema - 创建 Schema

##语法##

**db.createSchema(\<name\>, \<schemaDef\>)**

##类别##

Sdb

##描述##

该函数用于创建 Schema，并指定 Schema 的字段定义。

##参数##

- name（ *string，必填* ）

    Schema 的名称，需保证全局唯一

- schemaDef（ *object，必填* ）

    通过参数 schemaDef 可以指定 Schema 的字段定义，格式为 `{<Name1>: {<字段定义>}, <Name2>: {字段定义}, ...}`：

    - Name（ *string* ）：字段名
        
        格式：`{"id": {}}`

    - Type（ *string* ）：字段的数据类型

        格式：`{"id": {Type: "int32"}}`

    - ReadDefault（ *类型与字段 Type 的值对应* ）：读默认值，该参数选填

        通过 Information Schema 读取数据，当记录中不存在指定字段时需返回的默认值。

        格式：`{"id": {Type: "int32", ReadDefault: 0}}`

    - WriteDefault（ *类型与字段 Type 的值对应* ）：写默认值，该参数选填

        通过 Information Schema 写入数据，当写入的记录不存在指定字段时需补充的默认值。

        格式：`{"id": {Type: "int32", WriteDefault: 0}}`

##返回值##

函数执行成功时，将返回一个 SdbInfoSchema 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`createSchema()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -397 | SDB_SCHEMA_EXIST | Schema 已存在 | 检查是否存在同名的 Schema |
| -6 | SDB_INVALIDARG | 参数错误 | 检查参数类型是否填写正确 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.6.1 及以上版本

##示例##

创建名为“s1”的 Schema

```lang-javascript
> db.createSchema("s1", {"id": {Type: "int32", ReadDefault: 1}})
```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[SYSSCHEMAS]:manual/Manual/Catalog_Table/SYSSCHEMAS.md
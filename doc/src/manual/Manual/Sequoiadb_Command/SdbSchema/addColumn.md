[^_^]:
     addColumn

##名称##

addColumn - 新增 Schema 的字段

##语法##

**schema.addColumn(\<columnName\>, \<columnDef\>)**

##类别##

SdbInfoSchema

##描述##

该函数用于在 Schema 中新增字段。

##参数##

- columnName（ *string，必填* ）

    字段名

- columnDef （ *object，必填* ）

    通过参数 columnDef 可以选择需要修改的字段定义：

    - Type（ *string* ）：字段的数据类型

        格式：`Type: "int32"`

    - ReadDefault（ *类型与字段 Type 的值对应* ）：读默认值，该参数选填

        通过 Information Schema 读取数据，当记录中不存在指定字段时需返回的默认值。

        格式：`ReadDefault: 0`

    - WriteDefault（ *类型与字段 Type 的值对应* ）：写默认值，该参数选填

        通过 Information Schema 写入数据，当写入的记录不存在指定字段时需补充的默认值。

        格式：`WriteDefault: 0`

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

2. 为该 Schema 新增字段 id，并指定数据类型为 int32，读默认值为 0

    ```lang-javascript
    > schema.addColumn("id", {Type: "int32", ReadDefault: 0})
    ```

[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
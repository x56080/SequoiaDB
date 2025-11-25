##名称##

listLobPieces - 列举集合中的大对象分片信息

##语法##

**db.collectionspace.collection.listLobPieces()**

##类别##

SdbCollection

##描述##

该函数用于获取集合中的大对象的分片列表。

##返回值##

函数执行成功时，将返回一个 DBCursor 类型的对象。  

函数执行失败时，将抛出异常并输出错误信息。

##错误##

`listLobPieces()` 函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在 | 检查集合空间是否存在 |
| -23 | SDB_DMS_NOTEXIST| 集合不存在 | 检查集合是否存在 |

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][error_code]，或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。详细信息可参考[常见错误处理指南][faq]。

##版本##

此函数适用于 v5.8.4 及以上版本。

##示例##

* 列举 sample.employee 中所有的大对象分片

    ```lang-javascript
    > db.sample.employee.listLobPieces()
    {
      "Oid": {
        "$oid": "00006925cc8c3a00023465d8"
      },
      "Sequence": 1,
      "Length": 196387,
      "GroupID": 1000
    }
    {
      "Oid": {
        "$oid": "00006925cc8c3a00023465d8"
      },
      "Sequence": 0,
      "Length": 262144,
      "GroupID": 1000
    }
    {
      "Oid": {
        "$oid": "00006925cc983a00023465d9"
      },
      "Sequence": 1,
      "Length": 189915,
      "GroupID": 1000
    }
    {
      "Oid": {
        "$oid": "00006925cc983a00023465d9"
      },
      "Sequence": 0,
      "Length": 262144,
      "GroupID": 1000
    }
    Return 4 row(s).
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

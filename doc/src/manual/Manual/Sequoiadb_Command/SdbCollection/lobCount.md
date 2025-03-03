##名称##

lobCount - 统计当前集合符合条件的LOB总数

##语法##

**db.collectionspace.collection.lobCount([cond])**

**db.collectionspace.collection.lobCount([cond]).hint([hint])**

##类别##

SdbCollection

##描述##

该函数用于统计当前集合符合条件的LOB总数，可通过 hint 指定控制参数。

##参数##

参数 `cond` 的用法与 [find()][find] 的相同，参数 `hint` 的用法与 [listLobs()][listLobs] 的相同。

##返回值##

函数执行成功时，将返回一个 CLCount 类型的对象。通过该对象获取符合条件的LOB总数。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`count()`函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -6 | SDB_INVALIDARG | 参数错误 | 查看参数是否填写正确|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在| 检查集合空间是否存在|
| -23 | SDB_DMS_NOTEXIST| 集合不存在 | 检查集合是否存在|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v3.4.14/v5.8.4 及以上版本

##示例##

- 统计集合 sample.lob 所有的LOB数，即不指定参数 cond

    ```lang-javascript
    db.sample.lob.lobCount()
    ```
- 统计符合条件 Lob 大小大于 1MB 的LOB数

    ```lang-javascript
    > db.sample.lob.lobCount({Size: {$gt:1024*1024*1024}})
    ```

- 统计符合条件 Oid 为 "000067c20f5232000457d32a" 的LOB分片数

    ```lang-javascript
    > db.sample.lob.lobCount().hint({Oid:"000067c20f5232000457d32a",ListPieces:true})
    ```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[listLobs]:manual/Manual/Sequoiadb_Command/SdbCollection/listLobs.md
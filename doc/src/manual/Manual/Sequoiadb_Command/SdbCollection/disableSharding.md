
##名称##

disableSharding - 修改集合的属性关闭分区功能。

##语法##

**db.collectionspace.collection.disableSharding()**

##类别##

Collection

##描述##

修改集合的属性关闭分区功能。

##返回值##

成功：无。  

失败：抛出异常。

##错误##

`disableSharding()`函数常见异常如下：

| 错误码 | 错误类型 | 可能的原因 | 解决方法 |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | 选项暂不支持 | 检查当前集合属性，如果是分区集合不能修改与分区相关的属性。|

当异常抛出时，可以通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取[错误码](manual/Manual/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/faq.md)了解更多内容。

##版本##

v2.10及以上版本。

##示例##

1. 创建一个分区集合，然后将该集合的分区功能关闭

    ```lang-javascript
    > db.sample.createCL('employee', { ShardingKey : { a : 1 }, ShardingType : 'hash' } )
    > db.sample.employee.disableSharding()
    ```

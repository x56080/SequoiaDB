##名称##

enableCompression - 开启集合的压缩功能或者修改集合的压缩算法。

##语法##

**db.collectionspace.collection.enableCompression([options])**

##类别##

Collection

##描述##

开启集合的压缩功能或者修改集合的压缩算法。

##参数##

* `options` ( *Object*， *选填* )

    通过`options`参数可以修改集合属性：

    1. `CompressionType` ( *String* )：集合的压缩算法类型，默认为 lzw 算法。其可选取值如下：

        * "lzw"：使用 lzw 算法压缩。
        * "snappy"：使用 snappy 算法压缩。

        格式：`CompressionType : "lzw" | "snappy" `



##返回值##

成功：无。  

失败：抛出异常。

##错误##

`enableCompression()`函数常见异常如下：

| 错误码 | 错误类型 | 可能的原因 | 解决方法 |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | 选项暂不支持 | 检查当前集合属性，如果是分区集合不能修改与分区相关的属性。|

当异常抛出时，可以通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取[错误码](reference/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/faq.md)了解更多内容。

##版本##

v2.10及以上版本。

##示例##

1. 创建一个普通集合，然后将该集合修改为 snappy 压缩

    ```lang-javascript
    > db.foo.createCL('bar')
    > db.foo.bar.enableCompression( { CompressionType: 'snappy' } )
    ```


##名称##

alter - 修改集合空间的属性

##语法##

**db.collectionspace.alter(\<options\>)**

##类别##

Collection Space

##描述##

修改集合空间的属性。

请参考 [db.collectionspace.setAttributes\(\)](manual/Manual/Sequoiadb_Command/SdbCS/setAttributes.md)

##参数##

* `options` ( *Object*， *必填* )

    通过`options`参数可以修改集合空间属性：

    1. `PageSize` ( *Int32* )：数据页大小。单位为字节。

        * PageSize 只能选填 0，4096，8192，16384，32768，65536 之一。
        * PageSize 为 0 时，即默认值 65536。
        * 修改时不能有数据

        格式：`PageSize : <num>`

    2. `LobPageSize` ( *Int32* )：LOB 数据页大小。单位为字节。

        * LobPageSize 只能选填 0，4096，8192，16384，32768，65536，131072，262144，524288 之一。
        * LobPageSize 为 0 时，即默认值 262144。
        * 修改时不能有 LOB 数据

        格式：`LobPageSize : <num>`

    3. `Domain` ( *String* )：所属域。

        * 集合空间的数据必须分布在新指定域的组上

        格式：`Domain : <domain>`

##返回值##

函数执行成功时，无返回值

函数执行失败时，将抛异常并输出错误信息。

##错误##

`alter()`函数常见异常如下：

| 错误码 | 错误类型 | 可能的原因 | 解决方法 |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | 选项暂不支持 | 检查当前集合空间属性是否支持。|

当异常抛出时，可以通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取[错误码](manual/Manual/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)了解更多内容。

##版本##

v2.10 及以上版本。

##示例##

1. 创建一个集合空间，数据页大小为 4096，然后将该集合空间的数据页大小修改为 8192。

	```lang-javascript
 	> db.createCS( 'sample', { PageSize : 4096 } )
 	> db.sample.alter( { PageSize : 8192 } )
	```

2. 创建一个集合空间，然后为该集合空间指定一个域

    ```lang-javascript
    > db.createCS( 'sample' )
    > db.sample.alter( { Domain : 'domain' } )
    ```

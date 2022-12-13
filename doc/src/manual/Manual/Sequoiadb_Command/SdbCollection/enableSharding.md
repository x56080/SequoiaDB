##名称##

enableSharding - 修改集合的属性开启分区属性

##语法##

**db.collectionspace.collection.enableSharding(\<options\>)**

##类别##

SdbCollection

##描述##

该函数用于修改集合的属性开启分区属性。

##参数##

options ( *object，必填* )

通过 options 参数可以修改集合属性：

- ShardingKey ( *object，必填* )：分区键，取值为 1 或 -1，表示正向或逆向排序

    当集合仅存在于一个数据组中，或者集合没有挂载子集合时，ShardingKey 可以被修改。

    格式：`ShardingKey: {<字段1>: <1|-1>, [<字段2>: <1|-1>, ...]}`

- ShardingType ( *string* )：分区方式，默认值为"hash"

    取值如下：

    - "hash"：散列分区
    - "range"：范围分区

    当集合仅存在于一个数据组时，ShardingType 可以被修改。

    格式：`ShardingType: "range"`

- Partition ( *number* )：分区数，默认值为 4096

    - 该参数的取值必须是 2 的幂，取值范围为[2\^3，2\^20]。
    - 参数 ShardingType 的取值为"hash"时，该参数才能生效。
    - 当集合仅存在于一个数据组时，Partition 可以被修改。

    格式：`Partition: 512`

- AutoSplit ( *boolean* )：是否开启自动切分功能，默认值为 false，表示不开启自动切分

    - 参数 ShardingType 的取值为"hash"时，该参数才能生效。
    - 当集合仅存在于一个数据组时，AutoSplit 可以被修改。

    格式：`AutoSplit: true`

    >**Note:**
    >
    > 创建域和集合时均可指定参数 AutoSplit。如果显式指定集合的 AutoSplit，系统将优先按集合指定的值决定是否开启自动切分。

- EnsureShardingIndex ( *boolean* )：是否根据参数 ShardingKey 指定的字段自动创建名为"$shard"的索引，默认值为 true，表示自动创建

    当集合仅存在于一个数据组时，EnsureShardingIndex 可以被修改。
 
    格式：`EnsureShardingIndex: false`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`enableSharding()`函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -32 | SDB_OPTION_NOT_SUPPORT | 选项暂不支持 | 检查当前集合属性，如果是分区集合不能修改与分区相关的属性|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.10 及以上版本

##示例##

- 创建一个普通集合，然后将该集合修改为分区集合

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.enableSharding({ShardingKey: {a: 1}, ShardingType: "hash"})
    ```

- 创建一个普通集合，然后将该集合修改为分区集合，并且自动切分

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.enableSharding({ShardingKey: {a: 1}, ShardingType: "hash", AutoSplit: true})
    ```


[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
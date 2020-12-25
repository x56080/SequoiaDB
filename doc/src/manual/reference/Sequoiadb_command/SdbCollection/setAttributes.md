##名称##

setAttributes - 修改集合的属性

##语法##

**db.collectionspace.collection.setAttributes(\<options\>)**

##类别##

Collection

##描述##

当集合的属性不符合预期时，用户可以使用该函数修改集合的属性。

##参数##

options ( *object*， *必填* )

通过 options 参数可以修改集合属性，可组合使用 options 的如下选项：

1. ReplSize ( *number* )：写操作需同步的副本数。其可选取值如下：

    * -1：表示写请求需同步到该复制组若干活跃的节点之后，数据库写操作才返回应答给客户端 
    * 0：表示写请求需同步到该复制组的所有节点之后，数据库写操作才返回应答给客户端 
    * 1~7：表示写请求需同步到该复制组指定数量个节点之后，数据库写操作才返回应答给客户端 

    格式：`ReplSize: <num>`

2. ShardingKey ( *object* )：分区键

    格式：`ShardingKey:{<字段1> : <1|-1>,[<字段2> : <1|-1>, ...]}`

    * 集合中已有的 ShardingKey 会被修改成新的 ShardingKey。
    * 集合只能存在于一个数据组中，或者集合为没有挂载子表的主表。

3. ShardingType ( *string* )：分区方式，默认为散列分区，其可选取值如下：

    * "hash"：散列分区
    * "range"：范围分区

    格式：`ShardingType : "hash" | "range"`

4. Partition ( *number* )：分区数，仅当选择散列分区时填写，代表散列分区的个数，其值必须是 2 的幂，范围在[2\^3，2\^20]

    格式：`Partition : <分区数>`

5. AutoSplit ( *boolean* )：标识新集合是否开启自动切分功能，默认值为 false

    * 集合设置新的 hash 分区键后，可以使用该选项进行自动切分。
    * 不显式指定 AutoSplit 时，如果该集合修改前无指定 AutoSplit 且从属于某个非系统域，该域的 AutoSplit 参数将作用于此次设置；若集合之前有指定 AutoSplit 为 false，需要显式设置 AutoSplit 为 true 进行自动切分。
    * AutoSplit 只能作用于 hash 分区键上。

    格式：`AutoSplit : true | false`

6. EnsureShardingIndex ( *boolean* )：标识是否创建分区索引，默认值为 true

7. Compressed ( *boolean* )：标识集合是否开启数据压缩功能。

    * 如果设置 Compressed 为 true，而没有指定 CompressionType，则 CompressionType 为"lzw"

    格式：`Compressed : true | false`

8. CompressionType ( *string* )：集合的压缩算法，"snappy"或者"lzw"。

    * "snappy"：使用 snappy 算法压缩。
    * "lzw"：使用 lzw 算法压缩。

    格式：`CompressionType : "snappy" | "lzw"`

9. StrictDataMode ( *boolean* )：标识对该集合的操作是否开启严格数据类型模式

    格式：`StrictDataMode : true | false`

10. AutoIncrement ( *object* )：自增字段

    * option 中须加上 Field 属性，以标记要修改的字段。
    * 自增字段可以修改的属性有 CurrentValue、Increment、StartValue、MinValue、MaxValue、CacheSize、AcquireSize、Cycled 和 Generated，属性具体功能请参考[自增字段介绍][sequence]。
    * 修改属性后，字段值可能不唯一，如需保证修改后值唯一，建议使用唯一索引。

    格式：`AutoIncrement : <option>`

> **Note:**
>
> * 各个选项的具体使用方式见 [db.collectionspace.createCL()][createCL]。
> * 分区集合不能修改与分区相关的属性，如 ShardingKey、Partition 等。
> * EnsureShardingIndex 和 AutoSplit 仅对当前该次操作生效，仅当修改分区属性，如 ShardingKey 等。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛出异常并输出错误信息。

##错误##

`setAttributes()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能的原因 | 解决方法 |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | 选项暂不支持 | 检查当前集合属性，如果是分区集合不能修改与分区相关的属性 |

当异常抛出时，可以通过 [getLastError()][getLastError] 获取错误码， 
或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。更多错误处理可以参考[常见错误处理指南][faq]了解更多内容。

##版本##

v2.10及以上版本

##示例##

- 创建一个普通集合后将该集合修改为分区集合

    ```lang-javascript
    > db.foo.createCL('bar')
    > db.foo.bar.setAttributes( { ShardingKey: { a: 1 }, ShardingType: "hash" } )
    ```

- 创建一个普通集合后将该集合修改为分区集合，并且自动切分

    ```lang-javascript
    > db.foo.createCL('bar')
    > db.foo.bar.setAttributes( { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } )
    ```

- 创建一个普通集合后将该集合修改为 snappy 压缩

    ```lang-javascript
    > db.foo.createCL('bar')
    > db.foo.bar.setAttributes( { CompressionType: 'snappy' } )
    ```

- 创建一个有自增字段的集合后修改其自增起始值

    ```lang-javascript
    > db.foo.createCL( 'bar', { AutoIncrement: { Field: "studentID" } } )
    > db.foo.bar.setAttributes( { AutoIncrement: { Field: "studentID", StartValue: 2017140000 } } )
    ```


[^_^]:
    本文使用的所有引用及链接
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[createCL]:reference/Sequoiadb_command/SdbCS/createCL.md
[getLastError]:reference/Sequoiadb_command/Global/getLastError.md
[getLastErrMsg]:reference/Sequoiadb_command/Global/getLastErrMsg.md
[faq]:manual/faq.md

##名称##

alter - 修改集合的属性

##语法##

**db.collectionspace.collection.alter(\<options\>)**

##类别##

SdbCollection

##描述##

该函数用于修改集合的属性。

##参数##

options ( *object，必填* )

通过参数 options 可以修改集合属性：

- ReplSize ( *number* )：写操作需同步的副本数，默认值为 1，表示写操作只需写入主节点

    取值如下：

    - -1：写请求需同步到该复制组若干活跃的节点之后，数据库写操作才返回应答给客户端
    - 0：写请求需同步到该复制组的所有节点之后，数据库写操作才返回应答给客户端
    - 1~7：写请求需同步到该复制组指定数量个节点之后，数据库写操作才返回应答给客户端

    格式：`ReplSize: 0`

- ShardingKey ( *object* )：分区键，取值为 1 或 -1，表示正向或逆向排序

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

- Compressed ( *boolean* )：是否开启数据压缩功能，默认值为 true，表示开启数据压缩功能

    格式：`Compressed: false`

- CompressionType ( *string* )：压缩算法类型，默认值为"lzw"

    取值如下：

    - "snappy"：snappy 算法压缩
    - "lzw"：lzw 算法压缩

    格式：`CompressionType: "snappy"`

    >**Note:**
    >
    > snappy 压缩和 lzw 压缩的使用场景可参考[数据压缩][date_compression]。

- StrictDataMode ( *boolean* )：是否开启严格数据类型模式，默认值为 false，表示不开启

    开启严格模式后，如果数据类型为数值，在运算过程中出现溢出则会报错；如果数据类型非数值，则不进行任何操作。

    格式：`StrictDataMode: true`

- AutoIncrement ( *object* )：自增字段的属性

    - 允许修改的属性可参考[自增字段][sequence]。
    - 修改属性后，字段值将可能不唯一。如需保证修改后值唯一，建议使用唯一索引。

    格式：`AutoIncrement: {Field: <字段名>, ...}` 或 `AutoIncrement: [{Field: <字段名1>, ...}, {Field: <字段名2>, ...}, ...]`

- AutoIndexId ( *boolean* )：标识是否创建 $id 索引

    格式：`AutoIndexId: false`

- DSMainCLName ( *string* )：集合在[数据源][datasource]端所关联的主表名

    仅数据源表作为子集合挂载时有效。如果集合在数据源端所关联的主表名与实际不符，可以使用该选项进行修正。

    > **Note:**
    >
    > - 各个选项的具体使用方式见 [db.collectionspace.createCL()][createCL]。
    > - 分区集合不能修改与分区相关的属性，如 ShardingKey、Partition 等。
    > - EnsureShardingIndex 和 AutoSplit 仅对当前该次操作生效，仅当修改分区属性，如 ShardingKey 等时有效。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`alter()` 函数常见异常如下：

| 错误码 | 错误类型 | 可能发生的原因 | 解决办法 |
| ------ | -------- | -------------- | -------- |
| -32 | SDB_OPTION_NOT_SUPPORT | 选项暂不支持 | 检查当前集合属性，如果是分区集合不能修改与分区相关的属性|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v1.12 及以上版本

##示例##

- 创建一个普通集合，然后将该集合修改为分区集合

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.alter({ShardingKey: {a: 1}, ShardingType: "hash"})
    ```

- 创建一个普通集合，然后将该集合修改为分区集合，并且自动切分

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.alter({ShardingKey: {a: 1}, ShardingType: "hash", AutoSplit: true})
    ```

- 创建一个普通集合，然后将该集合修改为 snappy 压缩

    ```lang-javascript
    > db.sample.createCL("employee")
    > db.sample.employee.alter({CompressionType: "snappy"})
    ```

- 创建一个有自增字段的集合，修改其自增起始值

    ```lang-javascript
    > db.sample.createCL("employee", {AutoIncrement: {Field: "studentID"}})
    > db.sample.employee.alter({AutoIncrement: {Field: "studentID", StartValue: 2017140000}})
    ```


[^_^]:
    本文使用的所有引用和链接
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md#修改自增字段属性
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
[date_compression]:manual/Distributed_Engine/Architecture/compression_encryption.md
[datasource]:manual/Distributed_Engine/Architecture/datasource.md

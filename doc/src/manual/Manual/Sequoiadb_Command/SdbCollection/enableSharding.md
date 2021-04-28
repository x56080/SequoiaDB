##名称##

enableSharding - 修改集合的属性开启分区属性

##语法##

**db.collectionspace.collection.enableSharding(\<options\>)**

##类别##

SdbCollection

##描述##

该函数用于修改集合的属性开启分区属性。

##参数##

* `options` ( *Object*， *必填* )

    通过`options`参数可以修改集合属性，如指定集合的分区键等。可组合使用 `options` 的如下选项：

    1. `ShardingKey` ( *Object* )：分区键。（必填）

        格式：`ShardingKey:{<字段1> : <1|-1>,[<字段2> : <1|-1>, ...]}`

        * 已有的 ShardingKey 会被修改成新的 ShardingKey
        * 集合只能存在于一个数据组中，或者集合为没有挂载子表的主表

    2. `ShardingType` ( *String* )：分区方式，默认为 hash 分区。其可选取值如下：

        * "hash"：hash 分区。（默认值）
        * "range"：范围分区。

        格式：`ShardingType : "hash" | "range"`

        * 集合只能存在于一个数据组中

    3. `Partition` ( *Int32* )：分区数。仅当选择 hash 分区时填写，
                                代表了 hash 分区的个数。其值必须是2的幂。
                                范围在[2\^3，2\^20]。

        格式：`Partition : <分区数>`

        * 默认值是：4096
        * 集合只能存在于一个数据组中
    
    4. `AutoSplit` ( *Bool* )：标识是否开启自动切分功能

        格式：`AutoSplit : true | false`

        * 默认值是 false
        * 集合设置新的 hash 分区键后，可以使用该选项进行自动切分
        * 不显式指定 AutoSplit 时，如果该集合修改前无指定 AutoSplit 且从属于某个非系统域，该域的 AutoSplit 参数将作用于此次设置
        * 集合之前有指定 AutoSplit 为 false，需要显式设置 AutoSplit 为 true 进行自动切分
        * AutoSplit 只能作用于 hash 分区键上

    5. `EnsureShardingIndex` ( *Bool* )：标识是否创建分区索引

        * 默认值是 true

	**Note:**

    * 各个选项的具体使用方式见 [db.collectionspace.createCL()][createCL]。
	* 分区集合不能修改与分区相关的属性。
    * EnsureShardingIndex 和 AutoSplit 仅对当前该次操作生效，仅当修改分区属性，如 ShardingKey 等时有效

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`enableSharding()`函数常见异常如下：

| 错误码 | 错误类型 | 可能的原因 | 解决方法 |
| ------ | --- | ------------ | ----------- |
| -32 | SDB_OPTION_NOT_SUPPORT | 选项暂不支持 | 检查当前集合属性，如果是分区集合不能修改与分区相关的属性。|

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][error_code]，
或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。
可以参考[常见错误处理指南][faq]了解更多内容。

##版本##

v2.10 及以上版本。

##示例##

1. 创建一个普通集合，然后将该集合修改为分区集合：

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.enableSharding( { ShardingKey: { a: 1 }, ShardingType: "hash" } )
    ```

2. 创建一个普通集合，然后将该集合修改为分区集合，并且自动切分：

    ```lang-javascript
    > db.sample.createCL('employee')
    > db.sample.employee.enableSharding( { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } )
    ```


[^_^]:
     本文使用的所有引用及链接
[createCL]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md
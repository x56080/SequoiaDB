## 名称

createCL - 创建一个新的集合

## 语法

**db.collectionspace.createCL(\<name\>,[options])**

## 类别

Collection Space

## 描述

该函数用于在指定集合空间下创建集合（Collection），集合是数据库中存放文档记录的逻辑对象，任何一条文档记录必须属于一个且仅属于一个集合。

## 参数

* name（ *string*， *必填* ）

    集合名，在同一个集合空间中，集合名必须唯一


* options（ *object*， *选填* ）

    在创建集合时，可以通过 options 参数设置集合的其他属性，如指定集合的分区键，是否以压缩的形式插入数据等。可组合使用 options 的如下选项：

    1. ShardingKey（object）：分区键

        格式：`ShardingKey:{<字段1> : <1|-1>,[<字段2> : <1|-1>, ...]}`

    2. ShardingType（string）：分区方式，默认为 hash 分区
 
       其可选取值如下：

        * "hash"：hash 分区
        * "range"：范围分区

        格式：`ShardingType:"hash"|"range"`

    3. Partition（number）：分区数，仅当选择 hash 分区时填写，代表了 hash 分区的个数，默认为 4096

        该参数值必须是2的幂，范围在[2\^3，2\^20]。

        格式：`Partition: <分区数>`

    4. ReplSize（number）：写操作需同步的副本数，默认值为1

        其可选取值如下：

        * -1：表示写请求需同步到该复制组若干活跃的节点之后，数据库写操作才返回应答给客户端
        * 0：表示写请求需同步到该复制组的所有节点之后，数据库写操作才返回应答给客户端
        * 1 - 7：表示写请求需同步到该复制组指定数量个节点之后，数据库写操作才返回应答给客户端

        格式：`ReplSize: <num>`

    5. Compressed（boolean）：新集合是否开启数据压缩功能，默认为 true

        格式：`Compressed:true|false`

    6. CompressionType（string）：压缩算法类型，默认为 lzw 算法

        其可选取值如下：

        * "snappy"：使用 snappy 算法压缩
        * "lzw"：使用 lzw 算法压缩

        格式：`CompressionType:"snappy"|"lzw"`

    7. IsMainCL（boolean）：新集合是否为主分区集合（主表），默认为 false

        格式：`IsMainCL:true|false`

    8. AutoSplit（boolean）：新集合是否开启自动切分功能，默认为 false。

        格式：`AutoSplit:true|false`

    9. Group（string）：指定新集合将被创建到哪个复制组

        格式：`Group:<group name>`

    10. AutoIndexId（boolean）：新集合是否自动使用 _id 字段创建名字为"$id"的唯一索引，默认为 true
 
        格式：`AutoIndexId:true|false`

    11. EnsureShardingIndex（boolean）：集合是否自动使用 ShardingKey 包含的字段创建名字为"$shard"的索引，默认为 true

        格式：`EnsureShardingIndex:true|false`

    12. StrictDataMode（boolean）：对该集合的操作是否开启严格数据类型模式，默认为 false，不开启

        严格数据模式的开启标示对数值操作存在以下限制：

        * 运算过程不改数据类型；
        * 数值运算出现溢出时直接报错，错误码 SDB_VALUE_OVERFLOW；

      	格式：`StrictDataMode:true|false`

    13. AutoIncrement（object）：自增字段

        格式：`AutoIncrement:{Field: <字段>, ...}` 或 `AutoIncrement:[ {Field: <字段1>, ...}, {Field: <字段2>, ...}, ... ]`

        示例：`AutoIncrement: { Field: "userID", Generated: "always" }`

        参数详情可参考[自增字段介绍][sequence]
        
    14. LobShardingKeyFormat（string）：指定大对象生成主分区集合切分键键值的格式

        目前支持将大对象ID中的时间属性转换成如下字符串形式：
    
        * "YYYYMMDD"：将大对象ID的时间属性转换为年月日的字符串形式，如"20190701"
        * "YYYYMM"：将大对象ID的时间属性转换为年月的字符串形式，如"201907"
        * "YYYY"：将大对象ID的时间属性转换为年的字符串形式，如"2019"
    
        格式：`LobShardingKeyFormat:"YYYYMMDD"|"YYYYMM"|"YYYY"`


    15. DataSource（string）：指定所使用的数据源名称

        格式：`{DataSource: "ds1"}`

    16. Mapping（string）：所映射的集合名称

        格式：`{Mapping: "bar"}`

> **Note:**
>
> * 集合名限制请参考[限制][sequoiadb_limitation]
>
> * 当参数 `options` 内设置了多个参数时，需用英文半角的逗号","将各参数的取值隔开。
>
> * 在[创建集合空间][createCS]时，可以指定所属的[数据域][domain]。创建集合时，使用 Group 参数，指定的复制组必须在域内；不使用 Group 参数，集合将被创建在域的任意一个复制组上。
>
> * 创建集合的 `AutoSplit` 参数比数据域的 `AutoSplit` 属性优先级更高。
>
> * `AutoSplit` 不能与 `Group` 参数同时使用。
>
> * `AutoSplit` 必须配合散列分区使用。
>   
> * 压缩算法选择策略：snappy 压缩算法是以单条记录为单位进行压缩，记录内部的数据重复度直接影响到压缩率。因此，当记录内部数据重复度较高，如每条记录的字段名、字段值相似，使用 snappy 算法可获得良好的压缩性能。如果记录内部数据重复度很低，但记录间具有更高的相似性，如不同记录之间有相同的字段名，相近的字段值等，则使用 lzw 算法更优。
>    
> * `LobShardingKeyFormat` 只能在主分区集合中使用，同时要求切分键只能有一个切分字段。
>
> * 在使用主分区集合（主表）与子分区集合（子表）时，需要注意两者的属性使用关系： 
>     1. 当从主分区集合写入数据时，`ReplSize`、`AutoIncrement` 属性会沿用主分区集合的属性值。
>     2. 当从子分区集合写入数据时，`ReplSize`、`AutoIncrement` 属性会沿用子分区集合的属性值。
>     3. 集合的其他属性，如 `ShardingKey`、`Compressed`、`AutoIndexId` 等，子分区集合会使用自己的属性值而不是沿用主分区集合对应的属性值。
> * DataSource 和 Mapping 参数的具体使用场景可参考[数据源][datasource]。

## 返回值
 
函数执行成功时，返回一个新的 SdbCollection 对象。

函数执行失败时，将抛异常并输出错误信息。

## 错误

`createCL()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -2 | SDB_OOM | 无可用内存。| 检查物理内存及虚拟内存的情况。|
| -6 | SDB_INVALIDARG | 参数错误。 | 查看参数是否填写正确。|
| -22 | SDB_DMS_EXIST | 集合已存在。| 检查集合是否存在。|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在。| 检查集合空间是否存在。|
| -318 | SDB_VALUE_OVERFLOW | 数值运算出现溢出。| 检查运算过程是否存在溢出情况。|
当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][Sequoiadb_error_code]，
或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。
可以参考[常见错误处理指南][faq]了解更多内容。

## 版本

v1.0 及以上版本。

## 示例

1. 在集合空间 sample 下创建集合 employee，不指定分区键。

    ```lang-javascript
    > db.sample.createCL( "employee" )
    localhost:11810.sample.employee
    Takes 0.120450s.
    ```

2. 在集合空间 sample 下创建集合 employee。该集合若需要切分数据到其它复制组，将
   使用 age 字段进行 hash 切分；该集合默认开启了数据压缩功能，使用默认的 lzw 算法压缩数据；写操作作用于该集合时，只需写入主节点即可返回。

    ```lang-javascript
    > db.sample.createCL( "employee",{ ShardingKey:{ age: 1 }, ShardingType: "hash", Partition: 4096, ReplSize: 1 } )
    localhost:11810.sample.employee
    Takes 0.110319s.
    ```
3. 在集合空间 sample 下创建集合 employee，开启严格数据类型模式。

    ```lang-javascript
    > db.sample.createCL( "employee", { StrictDataMode: true } )
    localhost:11810.sample.employee
    Takes 0.120450s.
    ```
    
4. 在主分区集合下使用大对象
    * 在集合空间 sample 下创建支持大对象的主分区集合 maincl，同时关联子表 subcl。

    ```lang-javascript
    > db.sample.createCL("maincl", { LobShardingKeyFormat:"YYYYMMDD", ShardingKey:{ date:1 }, IsMainCL:true, ShardingType:"range" } )
    localhost:11810.sample.maincl
    Takes 0.058532s.
    > db.sample.createCL("subcl")
    localhost:11810.sample.subcl
    Takes 0.294612s.
    > db.sample.maincl.attachCL( "sample.subcl", { LowBound: { date: "20190701" }, UpBound: { date: "20190801" } } )
    Takes 0.008561s.
    ```
    
    * 在[20190701, 20190801)之间创建的大对象数据则会落在集合 sample.subcl 中

    ```lang-javascript
    > Timestamp()
    Timestamp("2019-07-23-18.04.07.539050")
    > db.sample.maincl.putLob('/opt/data/test.dat')
    00005d36dbee370002de8080
    Takes 0.246062s.
    ```
    
    * 也可以指定大对象ID的时间属性

    ```lang-bash
    > db.sample.maincl.createLobID("2019-07-23-18.04.07")
    00005d36db97360002de8081
    Takes 0.108365s.
    > db.sample.maincl.putLob('/opt/data/test.dat', '00005d36db97360002de8081')
    00005d36db97360002de8081
    Takes 0.002216s.
    ```


[^_^]:
     本文使用的所有引用及链接
[sequence]:manual/Distributed_Engine/Architecture/Data_Model/sequence.md
[sequoiadb_limitation]:manual/Manual/sequoiadb_limitation.md
[createCS]:manual/Manual/Sequoiadb_Command/Sdb/createCS.md
[domain]:manual/Distributed_Engine/Architecture/domain.md
[datasource]:manual/Distributed_Engine/Architecture/datasource.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/faq.md
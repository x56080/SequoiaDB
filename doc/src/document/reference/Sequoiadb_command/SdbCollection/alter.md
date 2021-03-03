##语法##
***db.collectionspace.collection.alter\(\<options\>\)***

修改集合的属性。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------   | -----| ------ |
| options | Json 对象 | 修改的属性。 | 是 |

###options 中可选的属性格式###

| 参数名 | 描述   | 格式   |
| ------ | ------ | ------ |
| ReplSize | 一次写请求完成副本数。 | ReplSize: \<int32\> |
| ShardingKey | 分区键。| ShardingKey:{\<字段1\>:\<1&#124;-1\>,[\<字段2\>:\<1&#124;-1\>, ...]} |
| ShardingType | 分区方式，默认为 hash 分区。 | ShardingType:"hash"&#124;"range" |
| Partition | 分区数，hash 分区时填写，代表了 hash 分区的个数。其值必须是2的幂。范围在[2^3 , 2^20]。 | Partition:\<分区数\>|

> **Note:**
>
> * ShardingKey，ShardingType，Partition 的使用方式见 db.collectionspace.createCL()。
> * 分区集合不能修改与分区相关的属性。
> * 修改为分区集合后需要手动进行 split。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。

##错误##

| 错误码 | 可能的原因   | 解决方法    |
| ------ | ------------ | ----------- |
| -32    | 选项暂不支持 | 检查当前集合属性，如果是分区集合不能修改与分区相关的属性。 |

##示例##

* 创建一个普通集合

 ```lang-javascript
 > db.foo.createCL('bar')
 ```
* 修改为分区集合

 ```lang-javascript
 > db.foo.bar.alter( { ShardingKey: { a: 1 }, ShardingType: "hash" } )
 ```

##语法##
***db.collectionspace.collection.createIndex\(\<name\>,\<indexDef\>,\[isUnique\],\[enforced\],\[sortBufferSize\])***

为集合创建[索引](basic_operation/indexes.md)，提高查询速度。

##参数描述##

| 参数名 | 参数类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| name | string | 索引名，同一个集合中的索引名必须唯一。 | 是 |
| indexDef | Json 对象 |  索引键，包含一个或多个指定索引字段与方向的对象。其方向为1代表从小到大排序，-1则为从大到小排序。 | 是 |
| isUnique | Boolean | 索引是否唯一，默认 false。设置为 true 时代表该索引为唯一索引。 | 否 |
| enforced | Boolean | 索引是否强制唯一，可选参数，在 isUnique 为 true 时生效，默认 false。设置为 true 时代表该索引在 isUnique 为 true 的前提下，不可存在一个以上全空的索引键。 | 否 |
| sortBufferSize | int | 创建索引时使用的排序缓存的大小，单位为MB。取值为0时表示不使用排序缓存。默认为64。| 否 |

> **Note:**
>
> * 在唯一索引所指定的索引键字段上，集合中不可存在一条以上的记录完全重复
> * 索引名不能是空串，含点（.）或者美元符号（$）。且长度不超过1023B
> * 索引的key最大大小为1000字符。如 {a:"fjsdlfjlsdfj....."}，如果a为索引字段，当插入记> 录含有a字段，并且该字段对应的value的大小超过1000字符时，无法插入该记录。
> * 索引字段的数量最多为32个。
> * 在集合记录数据量较大时（大于1000万条记录）适当增大排序缓存大小可以提高创建索引的速度

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 在集合 bar 下为字段名 age 创建名为 ageIndex 的唯一索引，记录按 age 字段值的升序排序。

 ```lang-javascript
 db.foo.bar.createIndex( "ageIndex", { age: 1 }, true )
 ```

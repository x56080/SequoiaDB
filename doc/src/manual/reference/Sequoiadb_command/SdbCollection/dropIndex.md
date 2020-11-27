##语法##
***db.collectionspace.collection.dropIndex\(\<name\>\)***

删除集合中指定的[索引](manual/Distributed_Engine/Architecture/Data_Model/index.md)。

## 参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| name   | string   | 索引名，同一个集合中的索引名必须唯一。 | 是 |

> **Note:**
>
> * 做删除索引操作时，索引名必须在集合中存在。
> * 索引名不能是空串，含点（.）或者美元符号（$），且长度不超过127B。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 删除集合 bar 下名为 ageIndex 的索引，假设 ageIndex 已存在。

 ```lang-javascript
 > db.foo.bar.dropIndex("ageIndex")
 ```

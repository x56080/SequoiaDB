##语法##
***db.collectionspace.collection.getIndex\(\<name\>\)***

获取当前集合中指定的索引信息。

## 参数描述##

| 参数名 | 参数类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| name   | string   | 索引名，同一个集合中的索引名必须唯一。 | 是 |

> **Note:**
>
> * 索引名必须在集合中存在。
> * 索引名不能是空串，含点（.）或者美元符号（$），且长度不超过127B。

##返回值##

返回指定索引的引用，类型为object。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

## 示例##

* 返回集合 bar 下名为 ageIndex 索引的引用，假设 ageIndex 已存在。

 ```lang-javascript
 > db.foo.bar.getIndex( "ageIndex" )
 ```

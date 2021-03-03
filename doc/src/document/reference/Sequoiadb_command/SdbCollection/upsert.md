##语法##
***db.collectionspace.collection.upsert\(\<rule\>,\[cond\],\[hint\],\[setOnInsert\]\)***

更新集合记录。upsert 方法跟 update 方法都是对记录进行更新，不同的是当使用 cond 参数在集合中匹配不到记录时，update 不做任何操作，而 upsert 方法会做一次插入操作。

##参数描述##

| 参数名 | 参数类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| rule | Json 对象 | 更新规则。记录按 rule 的内容更新。 | 是 |
| cond | Json 对象 | 选择条件。为空时，更新所有记录，不为空时，更新符合条件的记录。 | 否 |
| hint | Json 对象 | 指定访问计划。 | 否 |
| setOnInsert | Json 对象 | 在做插入操作时向插入的记录中追加字段。 | 否 |

> **Note:**
>
> * hint 参数是一个包含一个单一字段的 Json 对象，字段名会被忽略，而其字段值则指定为需要访问索引的名称，当字段值为 null 时，则遍历集合中所有的记录，它的格式为{"":null}或者{"":"<indexname>"}。
>
> * 当 cond 参数在集合中匹配不到记录时，upsert 会生成一条记录插入到集合中。记录生成规则为：首先从 cond 参数中取出 $et 和 $all 操作符对应的键值对，没有的时候生成空记录。然后使用 rule 规则对其做更新操作，最后加入 setOnInsert 中的键值对。
>
> * upsert 本版本暂不支持对表分区键（ShardingKey）字段更新，如果包含对分区键的更新操作，将自动剔除掉对分区键的更新，但其他字段更新生效，且不会发生错误。


##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。

##错误##

[错误码](reference/Sequoiadb_error_code.md)


## 示例##

假设集合 bar 中有两条记录：

```
{
  "_id": {
    "$oid": "516a76a1c9565daf06030000"
  },
  "age": 10,
  "name": "Tom"
}
{
  "_id": {
    "$oid": "516a76a1c9565daf06050000"
  },
  "a": 10,
  "age": 21
}
```

* 按指定的更新规则更新集合中所有记录，即设置 rule 参数，不设定 cond 和 hint 参数的内容。如下操作等效于使用 update 方法，更新集合 bar 中的所有记录，使用[$inc](reference/operator/update_operator/inc.md)将记录的 age 字段值加1，name 字段值更改为“Mike”，对不存在 name 字段的记录，[$set](reference/operator/update_operator/set.md) 操作符会将 name 字段和其设定的值插入到记录中，可使用 find 方法查看更新结果。

 ```lang-javascript
 > db.foo.bar.upsert( { $inc: { age: 1 }, $set: { name: "Mike" } } )
 {
      "_id": {
      "$oid": "516a76a1c9565daf06030000"
      },
      "age": 11,
      "name": "Mike"
 }
 {
      "_id": {
      "$oid": "516a76a1c9565daf06050000"
      },
      "a": 10,
      "age": 22,
      "name":"Mike"
 }
 ```

* 选择符合匹配条件的记录，对这些记录按更新规则更新，即设定 rule 和 cond 参数。如下操作使用[$exists](reference/operator/match_operator/exists.md)匹配存在 type 字段的记录，使用[$inc](reference/operator/update_operator/inc.md)将这些记录的 age 字段值加3。在上面给出的两条记录中，都没有 type 字段，此时，upsert 操作会插入一条新的记录，新记录只有 \_id 字段和 age 字段名，\_id 字段值自动生成，而 age 字段值为3。

 ```lang-javascript
 > db.foo.bar.upsert( { $inc: { age: 3 } }, { type: { $exists: 1 } } )
 {
      "_id": {
      "$oid": "516a76a1c9565daf06030000"
      },
      "age": 11,
      "name": "Mike"
 }
 {
      "_id": {
      "$oid": "516a76a1c9565daf06050000"
      },
      "a": 10,
      "age": 22,
      "name":"Mike"
 }
 {
      "_id": {
      "$oid": "516cfc334630a7f338c169b0"
      },
      "age": 3
 }
 ```

* 按访问计划更新记录，假设集合中存在指定的索引名 testIndex，此操作等效于使用 update 方法，使用索引名为 testIndex 的索引访问集合 bar 中 age 字段值大于20的记录，将这些记录的 age 字段名加1。

 ```lang-javascript
 > db.foo.bar.upsert( { $inc: { age: 1 } }, { age: { $gt: 20 } }, { "": "testIndex" } )
 {
      "_id": {
      "$oid": "516a76a1c9565daf06050000"
      },
      "a": 10,
      "age": 23,
      "name":"Mike"
 }
 ```

* 使用setOnInsert更新记录，由于集合 bar 中 age 字段值大于30的记录为空，upsert在做插入操作时向插入的记录中追加字段{"name":"Mike"}。

 ```lang-javascript
 > db.foo.bar.upsert( { $inc: { age: 1 } }, { age: { $gt: 30 } }, {}, { "name": "Mike" } )
 {
      "_id": {
      "$oid": "516a76a1c9565daf06050000"
      },
      "age":1,
      "name":"Mike"
 } 
 ```

##语法##
***db.collectionspace.collection.update\(\<rule\>, \[cond\], \[hint\], \[options\]\)***

更新集合记录。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| rule   | Json 对象| 更新规则。记录按 rule 的内容更新。 | 是 |
| cond   | Json 对象| 选择条件。为空时，更新所有记录，不为空时，更新符合条件的记录。 | 否 |
| hint   | Json 对象| 指定访问计划。 | 否 |
| options| Json 对象| 可选项，详见options选项说明。| 否 |

##options选项##

| 参数名          | 参数类型 | 描述                | 默认值 |
| --------------- | -------- | ------------------- | ------ |
| KeepShardingKey | bool     | 为 false 时，将不保留更新规则中的分区键字段，只更新非分区键字段。<br>为 true 时，将会保留更新规则中的分区键字段。| false  |

> **Note:**
>
> * 参数`hint`的用法与[find()](reference/Sequoiadb_command/SdbCollection/find.md)的相同。
>
> * 目前分区集合上，不支持更新分区键。如果 `KeepShardingKey` 为 true，并且更新规则中带有分区键字段，将会报错-178。


##返回值##
* 成功返回详细结果信息（BSONObj 对象），结构如下：

 ```lang-json
 {
		UpdatedNum  : <INT64>  成功更新的记录数，包括匹配但未发生数据变化的记录,
		ModifiedNum : <INT64>  成功更新且发生数据变化的记录数,
		InsertedNum : <INT32>  成功插入的记录数，仅在 upsert 下生效
 }
 ```

* 出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误信息码。错误信息对象包括详细结果信息。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](reference/Sequoiadb_error_code.md)。
  
| 错误码   | 可能的原因               | 解决方法                                     |
| -------- | ------------------------ | -------------------------------------------- |
| -178     | 分区集合上不支持更新分区键 | KeepShardingKey 设置为 false，不更新分区键 |

## 示例##

* 按指定的更新规则更新集合中所有记录，即设置 rule 参数，不设定 cond 和 hint 参数的内容。如下操作更新集合 bar 下的 age 字段，使用[$inc](reference/operator/update_operator/inc.md)将 age 字段的值增加1。

 ```lang-javascript
 > db.foo.bar.update( { $inc: { age: 1 } } )
 ```

* 选择符合匹配条件的记录，对这些记录按更新规则更新，即设定 rule 和 cond 参数。如下操作使用匹配符[$exist](reference/operator/match_operator/exists.md)匹配更新集合 bar 中存在 age 字段而不存在 name 字段的记录，使用$unset将这些记录的 age 字段删除。

 ```lang-javascript
 > db.foo.bar.update( { $unset: { age: "" } }, { age: { $exists: 1 }, name: { $exists: 0 } } )
 ```

* 按访问计划更新记录，假设集合中存在指定的索引名。如下操作使用索引名为 testIndex 的索引访问集合 bar 中 age 字段值大于20的记录，将这些记录的 age 字段名加1。

 ```lang-javascript
 > db.foo.bar.update( { $inc: { age: 1 } }, { age: { $gt: 20 } }, { "": "testIndex" } )
 ```

* 分区集合 foo.bar，分区键为 { a: 1 }，含有以下记录

 ```lang-javascript
 > db.foo.bar.find()
 {
   "_id": {
     "$oid": "5c6f660ce700db6048677154"
   },
   "a": 1,
   "b": 1
 }
 Return 1 row(s).
 ```
 
 指定 KeepShardingKey 参数：不保留更新规则中的分区键字段。只更新了非分区键 b 字段，分区键 a 字段的值没有被更新。
 
 ```lang-javascript
 > db.foo.bar.update( { $set: { a: 9, b: 9 } }, {}, {}, { KeepShardingKey: false } )
 {
   "UpdatedNum": 1,
   "ModifiedNum": 0,
   "InsertedNum": 0
 }
 Takes 0.038184s.
 >
 > db.foo.bar.find()
 {
   "_id": {
     "$oid": "5c6f660ce700db6048677154"
   },
   "a": 1,
   "b": 9
 }
 Return 1 row(s).
 Takes 0.006393s.
 ```
 
 指定 KeepShardingKey 参数：保留更新规则中的分区键字段。因为目前不支持更新分区键，所以会报错。
 
 ```lang-javascript
 > db.foo.bar.update( { $set: { a: 9 } }, {}, {}, { KeepShardingKey: true } )
 (nofile):0 uncaught exception: -178
 Sharding key cannot be updated
 Takes 0.002696s.
 ```


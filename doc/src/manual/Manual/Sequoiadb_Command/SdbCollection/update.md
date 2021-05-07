##名称##

update - 更新集合记录

##语法##

**db.collectionspace.collection.update\(\<rule\>, \[cond\], \[hint\], \[options\]\)**

##类别##

SdbCollection

##描述##

该函数用于更新集合记录。

##参数##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| rule   | Json 对象| 更新规则。记录按 rule 的内容更新。 | 是 |
| cond   | Json 对象| 选择条件。为空时，更新所有记录，不为空时，更新符合条件的记录。 | 否 |
| hint   | Json 对象| 指定访问计划。 | 否 |
| options| Json 对象| 可选项，详见options选项说明。| 否 |

options 选项：

| 参数名          | 参数类型 | 描述                | 默认值 |
| --------------- | -------- | ------------------- | ------ |
| KeepShardingKey | bool     | false：不保留更新规则中的分区键字段，只更新非分区键字段。<br>true：保留更新规则中的分区键字段。| false  |
| JustOne         | bool     | true：只更新一条符合条件的记录。<br>false：更新所有符合条件的记录。| false  |

> **Note:**
>
> * 参数`hint`的用法与 [find()][find] 的相同。
>
> * 目前分区集合上，不支持更新分区键。如果 `KeepShardingKey` 为 true，并且更新规则中带有分区键字段，将会报错-178。
> 
> * `JustOne`为 true 时，只能在单个分区、单个子表上执行。


##返回值##

* 函数执行成功时，将返回一个 BSONObj 类型的对象。通过该对象获取成功更新的记录数信息，字段说明如下：

| 字段名 | 类型 | 描述 |
|--------|------|------|
| UpdatedNum | int64 | 成功更新的记录数，包括匹配但未发生数据变化的记录 |
| ModifiedNum | int64 | 成功更新且发生数据变化的记录数 |
| InsertedNum | int64 | 成功插入的记录数 |

* 函数执行失败时，将抛异常并输出错误信息。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码][error_code]。
  
| 错误码   | 可能的原因               | 解决方法                                     |
| -------- | ------------------------ | -------------------------------------------- |
| -178     | 分区集合上不支持更新分区键 | KeepShardingKey 设置为 false，不更新分区键 |
| -345     | JustOne 跨多个分区或者多个子表 | 修改匹配条件，或者不使用 JustOne |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 按指定的更新规则更新集合中所有记录，即设置 rule 参数，不设定 cond 和 hint 参数的内容。如下操作更新集合 employee 下的 age 字段，使用 [$inc][inc] 将 age 字段的值增加1。

    ```lang-javascript
    > db.sample.employee.update( { $inc: { age: 1 } } )
    ```

* 选择符合匹配条件的记录，对这些记录按更新规则更新，即设定 rule 和 cond 参数。如下操作使用匹配符 [$exist][exists] 匹配更新集合 employee 中存在 age 字段而不存在 name 字段的记录，使用$unset将这些记录的 age 字段删除。

    ```lang-javascript
    > db.sample.employee.update( { $unset: { age: "" } }, { age: { $exists: 1 }, name: { $exists: 0 } } )
    ```

* 按访问计划更新记录，假设集合中存在指定的索引名。如下操作使用索引名为 testIndex 的索引访问集合 employee 中 age 字段值大于20的记录，将这些记录的 age 字段名加1。

    ```lang-javascript
    > db.sample.employee.update( { $inc: { age: 1 } }, { age: { $gt: 20 } }, { "": "testIndex" } )
    ```

* 分区集合 sample.employee，分区键为 { a: 1 }，含有以下记录

    ```lang-javascript
    > db.sample.employee.find()
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
    > db.sample.employee.update( { $set: { a: 9, b: 9 } }, {}, {}, { KeepShardingKey: false } )
    {
      "UpdatedNum": 1,
      "ModifiedNum": 0,
      "InsertedNum": 0
    }
    > db.sample.employee.find()
    {
      "_id": {
        "$oid": "5c6f660ce700db6048677154"
      },
      "a": 1,
      "b": 9
    }
    Return 1 row(s).
    ```
 
    指定 KeepShardingKey 参数：保留更新规则中的分区键字段。因为目前不支持更新分区键，所以会报错。
 
    ```lang-javascript
    > db.sample.employee.update( { $set: { a: 9 } }, {}, {}, { KeepShardingKey: true } )
    (nofile):0 uncaught exception: -178
    Sharding key cannot be updated
    ```


[^_^]:
    本文使用的所有引用及链接
[find]:manual/Manual/Sequoiadb_Command/SdbCollection/find.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[inc]:manual/Manual/Operator/Update_Operator/inc.md
[exists]:manual/Manual/Operator/Match_Operator/exists.md
[error_guide]:manual/FAQ/faq_sdb.md

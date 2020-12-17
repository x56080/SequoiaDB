##语法##

***query.update( \<rule\>, [returnNew], [options] )***

##类别##

SdbQuery

##描述##

更新查询后的结果集。

> **Note:**  

> 1. 不能与 query.count()、query.remove()同时使用。  

> 2. 与 query.sort()同时使用时，在单个节点上排序必须使用索引。
  
> 3. 在集群中与 query.limit()或 query.skip()同时使用时，要保证查询条件会在单个分区或单个子表上执行。

##参数##

| 参数名    | 参数类型 | 默认值 | 描述                         | 是否必填 |
| --------- | -------- | ------ | ---------------------------- | -------- |
| rule      | JSON     | ---    | 更新规则，记录按指定规则更新 | 是       |
| returnNew | bool     | false  | 是否返回更新后的记录         | 否       |
| options   | JSON     | ---    | 可选项                       | 否       |

options 参数详细说明如下：

| 属性            | 值类型 | 默认值 | 描述 | 是否<br>必填 |
| --------------- | ------ | ------ | ---- | ------------ |
| KeepShardingKey | bool   | false  | 是否保留分区键字段 | 否 |

> **Note:**  

> 1. query.update()方法的定义格式包含 rule 参数、 returnNew 参数和options 参数。其中 rule 参数与 [update()](reference/Sequoiadb_command/SdbCollection/update.md)的 rule 参数相同，options 参数与 [update()](reference/Sequoiadb_command/SdbCollection/update.md)的 options 参数相同。

> 2. returnNew 参数默认为 false，当为 true 时，返回修改后的记录值。

##返回值##

返回结果集的游标。returnNew为false，则返回更新前的查询结果集的游标；returnNew为true，则返回更新后的结果集的游标。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 查询集合 bar 下 age 字段值大于10的记录（如使用 [$gt](reference/operator/match_operator/gt.md) 查询），并将符合条件的记录的 age 字段加1。

  ```lang-javascript
  > db.foo.bar.find( { age: { $gt: 10 } } ).update( { $inc: { age: 1 } } )
  {
      "_id": {
        "$oid": "5d006c45e846796ae69f85a9"
      },
      "age": 21,
      "name": "tom"
  }
  {
      "_id": {
        "$oid": "5d006c45e846796ae69f85aa"
      },
      "age": 22,
      "name": "ben"
  }
  {
      "_id": {
        "$oid": "5d006c45e846796ae69f85ab"
      },
      "age": 23,
      "name": "alice"
  }
  ```

##语法##
***query.hint( \<hint\> )***

按指定的索引遍历结果集。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| hint | Json 对象 | 指定访问计划，加快查询速度。 | 否 |

> **Note：**  
> query.hint() 的方法定义包含 hint 参数，如果不设定 hint 参数的内容相当于不使用索引遍历结果集。hint 参数是一个包含一个单一字段的 Json 对象，字段名会被忽略，而其字段值则指定为需要访问的索引名称，当字段值为 null 时，则遍历集合中所有的记录。  
> 格式：  ```{ "": null } 或者 { "": "<indexname>" } ```

##返回值##

返回查询结果集的游标，类型为 object 。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

使用索引 ageIndex 遍历集合 bar 下存在 age 字段的记录，并返回。

```lang-javascript
> db.foo.test.find( {age: {$exists:1} } ).hint( { "": "ageIndex" } )
{
  "_id": {
    "$oid": "5812feb6c842af52b6000007"
  },
  "age": 10
}
{
  "_id": {
    "$oid": "5812feb6c842af52b6000008"
  },
  "age": 20
}

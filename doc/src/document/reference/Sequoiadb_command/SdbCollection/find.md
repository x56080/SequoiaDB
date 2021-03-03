##语法##
***db.collectionspace.collection.find\(\[cond\],\[sel\]\)***

选择集合记录，对选择的记录返回一个游标（cursor）。在 SequoiaDB中 游标是一个指针，指向一个查询结果集，客户端可以遍历检索结果。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | -------- | ---- | -------- |
| cond   | Json 对象| 选择条件。为空时，查询所有记录，不为空时，查询符合条件记录。|否 |
| sel    | Json 对象| 控制返回记录的字段名。为空时，返回记录的所有字段，如果指定的字段名记录中不存在，则按用户设定的内容原样返回。 | 否 |

> **Note:**
>
> * 指定cond字段时可使用对应[匹配符](reference/operator/match_operator/overview.md)查询。
> * sel 是一个 Json 对象，字段的值一般设定为空。而如果指定值：{"字段名1":"值1","字段名2":"值2",...}，如果记录中存在所选字段，设定的值（值1，值2...）不生效；如果记录中不存在所选字段，则按指定的值输出。

##返回值##

返回游标。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 查询所有记录，不指定 cond 和 sel 字段。

 ```lang-javascript
 > db.foo.bar.find()
 ```

* 查询匹配条件的记录，即设置 cond 参数的内容。如下操作返回集合 bar 中符合条件 age 字段值大于25且 name 字段值为“Tom”的记录。

 ```lang-javascript
 > db.foo.bar.find( { age: { $gt: 25 }, name: "Tom" } )
 ```

* 指定返回的字段名，即设置 sel 参数的内容。如有记录{ age: 25, type: "system" }和{ age: 20, name: "Tom", type: "normal" }，如下操作返回记录的age字段和name字段。

 ```lang-javascript
 > db.foo.bar.find( null, { age: "", name: "" } )
 {
      "age": 25,
      "name": ""
 }
 {
      "age": 20,
      "name": "Tom"
 }
 ```


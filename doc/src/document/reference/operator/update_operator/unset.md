##语法##

```lang-json
{ $unset: { <字段名1>: "", <字段名2>: "", ... } }
```

##描述##

$unset操作是删除集合中指定的字段名。如果记录中没有指定的字段名，跳过。

##示例##

* 删除集合bar下记录的name字段和age字段，如果记录中没有字段name或age，跳过，不做任何处理

 ```lang-javascript
 > db.foo.bar.update({ $unset: { name: "", age: "" } })
 ```

* $unset删除数组对象中的元素。如有一条记录：

 ```lang-json
 { arr: [1,2,3], name: "Tom" }
 ```

 使用$unset删除第二个元素操作如下：

 ```lang-javascript
 > db.foo.bar.update({ $unset: { "arr.2": "" } })
 ```

 此操作后，记录更新为

 ```lang-json
 { arr: [1,null,3], name: "Tom" }
 ```

* $unset删除嵌套对象中的字段。如有一条记录：

 ```lang-json
 { content: { ID: 1, type: "system", position: "manager" }, name: "Tom" }
 ```

 content是一个嵌套对象，它有ID，type，position三个字段。使用$unset删除 type 字段操作如下：

 ```lang-javascript
 > db.foo.bar.update({ $unset: { "content.type": "" } })
 ```

 此操作后，记录更新为

 ```lang-json
 { content: { ID: 1, position: "manager" }, name: "Tom" }
 ```

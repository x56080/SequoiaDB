##语法##

```
{ $project: { <字段名1：0 | 1 | "$新字段名1">, [字段名2: 0 | 1 | "$新字段名2", ... ] } }
```

##描述##

$project类似SQL中的select语句，通过使用$project操作可以从记录中筛选出所需字段：

- 如果字段的值为1，表示选出
- 如果字段的值为0，表示不选
- 通过 "$新字段名" 可以实现把字段重命名为 "新字段名"

> **Note:**
>
> 如果记录不存在所选字段，则以如下格式输出："field":null，其中 field 为不存在的字段名。
>
> 对嵌套对象使用点操作符（.）引用字段名。

##示例##

- 使用$project快速地从结果集中选取所需字段

 ```lang-javascript
 > db.foo.bar.aggregate({ $project: { title: 0, author: 1 } })
 ```

 此操作是选出author字段，而title字段在结果集中不输出。

- 使用$project重命名字段名

 ```lang-javascript
 > db.foo.bar.aggregate({ $project : { author: 1, name: "$studentName", dep: "$info.department" } })
 ```

 此操作将字段名studentName重命名为name输出，将info对象中的子对象department字段重命名为dep。对嵌套对象，字段引用使用点操作符（.）指向。

- 使用$project选择输出字段，然后使用$match按条件匹配记录

 ```lang-javascript
 > db.foo.bar.aggregate({ $project: { score: 1, author: 1 } }, { $match: { score: { $gt: 80 } } })
 ```

 此操作使用$project输出所有记录的score和author字段值，然后按$match输出匹配条件的记录。

 > **Note:**
 > 由于$project选取了输出字段名，所以$match中字段名必须是$project中选出的字段名。

匹配符可以指定匹配条件，使查询仅返回符合条件的记录。同时支持搭配[函数操作][overview]使用，实现更复杂的查询操作。

- 单独使用匹配符，根据指定条件查询记录

    查询集合 sample.employee 中字段 age 取值大于 20 的记录

    ```lang-javascript
    > db.sample.employee.find({age: {$gt: 20}})
    ```

- 搭配函数操作使用，先对字段值进行函数运算，再将运算结果作为匹配条件进行查询

    查询集合 sample.employee 中字段 a 绝对值为 2 的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$abs: 1, $et: 2}})
    ```

所支持的匹配符如下：

| 匹配符 | 描述 | 示例 |
| ------ | ---- | ---- |
| [$gt][gt]               | 大于指定值       | db.sample.employee.find({age: {$gt: 20}}) |
| [$gte][gte]             | 大于等于指定值   | db.sample.employee.find({age: {$gte: 20}}) |
| [$lt][lt]               | 小于指定值       | db.sample.employee.find({age: {$lt: 20}}) |
| [$lte][lte]             | 小于等于指定值   | db.sample.employee.find({age: {$lte: 20}}) |
| [$ne][ne]               | 不等于指定值     | db.sample.employee.find({age: {$ne: 20}}) |
| [$et][et]               | 等于指定值       | db.sample.employee.find({"id": {"$et": 1}}) |
| [$mod][mod]             | 取模后等于指定值 | db.sample.employee.find({"age": {"$mod": [5, 3]}}) |
| [$and][and]             | 使用逻辑“与”连接表达式并匹配记录 | db.sample.employee.find({$and: [{age: 20}, {name: "Tom"}]}) |
| [$or][or]               | 使用逻辑“或”连接表达式并匹配记录 | db.sample.employee.find({$or: [{age: 20}, {name: "Tom"}]}) |
| [$not][not]             | 使用逻辑“非”连接表达式并匹配记录 | db.sample.employee.find({$not: [{age: 20}, {name: "Tom"}]}) |
| [$regex][regex]         | 匹配满足正则表达式的记录 | db.sample.employee.find({str: {$regex: 'dh.*fj', $options:'i'}}) |
| [$elemMatch][elemMatch] | 匹配满足所有条件的记录   | db.sample.employee.find({content: {$elemMatch: {name: "Jack", phone: "123"}}}) |
| [$exists][exists]       | 匹配存在指定字段的记录   | db.sample.employee.find({age: {$exists: 1}}) |
| [$isnull][isnull]       | 匹配存在指定字段且取值不为空的记录 | db.sample.employee.find({age: {$isnull: 0}}) |
| [$in][in]               | 匹配存在指定值的记录     | db.sample.employee.find({age: {$in: [20, 21]}}) |
| [$nin][nin]             | 匹配不存在指定值的记录   | db.sample.employee.find({age: {$nin: [20, 21]}}) |
| [$all][all]             | 匹配存在所有指定值的记录 | db.sample.employee.find({name: { $all: ["Tom", "Mike"]}}) |
| [$+标识符][identifier]  | 数组元素匹配             | db.sample.employee.find({"array.$2": 10}) 
| [$expand][expand]       | 数组展开成多条记录       | db.sample.employee.find({a: {$expand: 1}}) |
| [$returnMatch][returnMatch] | 返回匹配的数组元素   | db.sample.employee.find({a: {$returnMatch: 0, $in: [1, 4, 7]}}) |

[^_^]:
    本文使用的所有引用及链接
[overview]:manual/Manual/Operator/Function_Operator/Readme.md
[gt]:manual/Manual/Operator/Match_Operator/gt.md
[gte]:manual/Manual/Operator/Match_Operator/gte.md
[lt]:manual/Manual/Operator/Match_Operator/lt.md
[lte]:manual/Manual/Operator/Match_Operator/lte.md
[lte]:manual/Manual/Operator/Match_Operator/lte.md
[ne]:manual/Manual/Operator/Match_Operator/ne.md
[in]:manual/Manual/Operator/Match_Operator/in.md
[nin]:manual/Manual/Operator/Match_Operator/nin.md
[all]:manual/Manual/Operator/Match_Operator/all.md
[and]:manual/Manual/Operator/Match_Operator/and.md
[not]:manual/Manual/Operator/Match_Operator/not.md
[or]:manual/Manual/Operator/Match_Operator/or.md
[exists]:manual/Manual/Operator/Match_Operator/exists.md
[elemMatch]:manual/Manual/Operator/Match_Operator/elemMatch.md
[identifier]:manual/Manual/Operator/Match_Operator/identifier.md
[regex]:manual/Manual/Operator/Match_Operator/regex.md
[mod]:manual/Manual/Operator/Match_Operator/mod.md
[et]:manual/Manual/Operator/Match_Operator/et.md
[isnull]:manual/Manual/Operator/Match_Operator/isnull.md
[expand]:manual/Manual/Operator/Match_Operator/expand.md
[returnMatch]:manual/Manual/Operator/Match_Operator/returnMatch.md

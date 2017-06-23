
##匹配符##

匹配符可以指定匹配条件，使查询仅返回符合条件的记录。它还能跟[函数操作](reference/operator/function_operator/overview.md)配合使用，以实现更复杂的匹配操作。

匹配符列表如下：

| 匹配符 | 描述 | 示例 |
| ------ | ---- | ---- |
| [$gt](reference/operator/match_operator/gt.md)               | 大于           | db.foo.bar.find( { age: { $gt: 20 } } )                               |
| [$gte](reference/operator/match_operator/gte.md)             | 大于等于       | db.foo.bar.find( { age: { $gte: 20 } } )                              |
| [$lt](reference/operator/match_operator/lt.md)               | 小于           | db.foo.bar.find( { age: { $lt: 20 } } )                               |
| [$lte](reference/operator/match_operator/lte.md)             | 小于等于       | db.foo.bar.find( { age: { $lte: 20 } } )                              |
| [$ne](reference/operator/match_operator/ne.md)               | 不等于         | db.foo.bar.find( { age: { $ne: 20 } } )                               |
| [$in](reference/operator/match_operator/in.md)               | 集合内存在     | db.foo.bar.find( { age: { $in: [ 20, 21 ] } } )                          |
| [$nin](reference/operator/match_operator/nin.md)             | 集合内不存在   | db.foo.bar.find( { age: { $nin: [ 20, 21 ] } } )                         |
| [$all](reference/operator/match_operator/all.md)             | 全部           | db.foo.bar.find( { age: { $all: [ 20, 21 ] } } )                         |
| [$and](reference/operator/match_operator/and.md)             | 与             | db.foo.bar.find( { $and: [ { age: 20 }, { name: "Tom" } ] } )               |
| [$not](reference/operator/match_operator/not.md)             | 非             | db.foo.bar.find( { $not: [ { age: 20 }, { name: "Tom" } ] } )                 |
| [$or](reference/operator/match_operator/or.md)               | 或             | db.foo.bar.find( { $or: [ { age: 20 }, { name: "Tom" } ] } )                  |
| [$type](reference/operator/match_operator/type.md)           | 已废弃         | 无                                                            |
| [$exists](reference/operator/match_operator/exists.md)       | 存在           | db.foo.bar.find( { age: { $exists: 1 } } )                            |
| [$elemMatch](reference/operator/match_operator/elemMatch.md) | 元素匹配       | db.foo.bar.find( { content: { $elemMatch: { age: 20 } } } )                        |
| [$+标识符](reference/operator/match_operator/identifier.md)  | 数组元素匹配   | db.foo.bar.find( { "array.$2": 10 } )                              |
| [$size](reference/operator/match_operator/size.md)           | 已废弃         | 无                                                            |
| [$regex](reference/operator/match_operator/regex.md)         | 正则表达式     | db.foo.bar.find( { str: { $regex:  'dh, * fj', $options:'i' } } )      |

数组属性操作

|数组属性操作 | 描述 | 示例 |
| ----------- | ---- | ---- |
| [$expand](reference/operator/match_operator/expand.md) | 数组展开成多条记录 | db.foo.bar.find( { a: { $expand: 1 } } ) |
| [$returnMatch](reference/operator/match_operator/returnMatch.md) | 返回匹配的数组元素 | db.foo.bar.find( { a: { $returnMatch: 0, $in: [ 1, 4, 7 ] } } ) |



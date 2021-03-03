
##选择符##

选择符可以实现对查询的结果字段进行拆分、筛选等功能。在选择符中使用[函数操作](reference/operator/function_operator/overview.md)还能实现对结果的重写和转换等操作。

支持的选择符如下：

| 选择符                                                          | 描述                     | 示例                                              |
| --------------------------------------------------------------- | ------------------------ | ------------------------------------------------- |
| [$include](reference/operator/selector_operator/include.md)     | 选择或移除某个字段       | db.foo.bar.find( {}, { "a": { "$include": 1 } } )             |
| [$default](reference/operator/selector_operator/default.md)     | 当字段不存在时返回默认值 | db.foo.bar.find( {}, { "a": { "$default": "myvalue" } } )     |
| [$elemMatch](reference/operator/selector_operator/elemMatch.md) | 返回数组内满足条件的元素的集合 | db.foo.bar.find( {}, { "a": { "$elemMatch": { "a": 1 } } } ) |
| [$elemMatchOne](reference/operator/selector_operator/elemMatchOne.md) | 返回数组内满足条件的第一个元素的集合 | db.foo.bar.find( {}, { "a": { "$elemMatchOne": { "a": 1 } } } )             |


同时[函数操作](reference/operator/function_operator/overview.md)也可以作为选择符使用。
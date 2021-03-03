
##函数操作##

函数操作可以配合[匹配符](reference/operator/match_operator/overview.md)和[选择符](reference/operator/selector_operator/overview.md)使用，以实现更复杂的功能。

1. 配合匹配符一起使用，可以对字段进行各种函数运算之后，再执行匹配操作。

  以下示例，匹配字段a长度为3的记录：

  ```lang-javascript
  > db.foo.bar.find({a:{$strlen:1, $et:3}})
  ```

  > **Note:** 先获取字段a的长度，再用该长度与3比较，返回长度为3的记录。

2. 作为选择符使用，可以对选取的字段进行函数运算，返回运算后的结果。

  以下示例，返回将字段a转大写的结果：

  ```lang-javascript
  > db.foo.bar.find({}, {a:{$upper:1}})
  ```   

所有支持的函数操作如下：

| 函数                                                           | 描述             | 示例                                     |
| -------------------------------------------------------------- | ---------------- | ---------------------------------------- |
| [$abs](reference/operator/function_operator/abs.md)            | 取绝对值         | db.foo.bar.find({}, {a:{$abs:1}})        |
| [$ceiling](reference/operator/function_operator/ceiling.md)    | 向上取整         | db.foo.bar.find({}, {a:{$ceiling:1}})    |
| [$floor](reference/operator/function_operator/floor.md)        | 向下取整         | db.foo.bar.find({}, {a:{$floor:1}})      |
| [$mod](reference/operator/function_operator/mod.md)            | 取模运算         | db.foo.bar.find({}, {a:{$mod:1}})        |
| [$add](reference/operator/function_operator/add.md)            | 加法运算         | db.foo.bar.find({}, {a:{$add:10}})       |
| [$subtract](reference/operator/function_operator/subtract.md)  | 减法运算         | db.foo.bar.find({}, {a:{$subtract:10}})  |
| [$multiply](reference/operator/function_operator/multiply.md)  | 乘法运算         | db.foo.bar.find({}, {a:{$multiply:10}})  |
| [$divide](reference/operator/function_operator/divide.md)      | 除法运算         | db.foo.bar.find({}, {a:{$divide:10}})    |
| [$substr](reference/operator/function_operator/substr.md)      | 截取子串         | db.foo.bar.find({}, {a:{$substr:[0,4]}}) |
| [$strlen](reference/operator/function_operator/strlen.md)      | 获取字符串长度   | db.foo.bar.find({}, {a:{$strlen:10}})    |
| [$lower](reference/operator/function_operator/lower.md)        | 字符串转为小写   | db.foo.bar.find({}, {a:{$lower:1}})      |
| [$upper](reference/operator/function_operator/upper.md)        | 字符串转为大写   | db.foo.bar.find({}, {a:{$upper:1}})      |
| [$ltrim](reference/operator/function_operator/ltrim.md)        | 去除左侧空格     | db.foo.bar.find({}, {a:{$ltrim:1}})      |
| [$rtrim](reference/operator/function_operator/rtrim.md)        | 去除右侧空格     | db.foo.bar.find({}, {a:{$rtrim:1}})      |
| [$trim](reference/operator/function_operator/trim.md)          | 去除左右两侧空格 | db.foo.bar.find({}, {a:{$trim:1}})       |
| [$cast](reference/operator/function_operator/cast.md)          | 转换字段类型     | db.foo.bar.find({}, {a:{$cast:"int32"}}) |
| [$size](reference/operator/function_operator/size.md)          | 获取数组元素个数 | db.foo.bar.find({}, {a:{$size:1}}) |
| [$type](reference/operator/function_operator/type.md)          | 获取字段类型     | db.foo.bar.find({}, {a:{$type:1}}) |
| [$slice](reference/operator/function_operator/slice.md)        | 截取数组元素     | db.foo.bar.find({}, {a:{$slice:[0,2]}}) |

函数操作可以支持流水线式处理，多个函数流水线执行：

```lang-javascript
> db.foo.bar.find({a:{$trim:1, $upper:1, $et:"ABC"}})
```

> **Note:**
>
>先对字段a去除左右两侧空格，然后再转换成大写，最后匹配与"ABC"相等的记录

当字段类型为数组类型时，函数会对该字段做一次展开，并对每个数组元素执行函数操作。

以取绝对函值函数为例：

```lang-javascript
> db.foo.bar.find()
{
  "a": [
    1,
    -3,
    -9
  ]
}

> db.foo.bar.find({}, {a:{$abs:1}})
{
  "a": [
    1,
    3,
    9
  ]
}
```


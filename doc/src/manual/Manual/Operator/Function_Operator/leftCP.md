##语法##

```lang-json
{<字段名>: {$leftCP: <值>}}
```

##说明##

$leftCP 用于从字符串开头截取指定长度的子串，即截取指定[代码点][codePoint]数的子串。当字段类型为字符串时，将按 UTF-8 编码规则对字符串进行截取；当字段类型为数组时，将对每个数组元素进行截取；当字段类型为非字符串时，将返回 null。

{$leftCP: <值>} 中 <值> 的取值为正整数，表示截取的长度。如果取值为 0 或负数，将返回空串。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: "12三四五"})
```

- 作为选择符使用，在字段 a 中截取长度为 3 的子串并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$leftCP: 3}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "12三"
    }
    Return 1 row(s).
    ```

- 与匹配符配合使用，匹配字段 a 截取子串后值为“12三”的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$leftCP: 3, $et: "12三"}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "12三四五"
    }
    Return 1 row(s).
    ```

[^_^]:
  本文使用的所有引用及链接
[codePoint]:http://www.unicode.org/glossary/#code_point



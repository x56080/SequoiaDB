##语法##

```lang-json
{<字段名>: {$rightCP: <值>}}
```

##说明##

$rightCP 用于从字符串末尾截取指定长度的子串，即截取指定[代码点][codePoint]数的子串。当字段类型为字符串时，将按 UTF-8 编码规则对字符串进行截取；当字段类型为数组时，将对每个数组元素进行截取；当字段类型为非字符串时，将返回 null。

{$rightCP: <值>} 中 <值> 的取值为正整数，表示截取的长度。如果取值为 0 或负数，将返回空串。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: "12三四五"})
```

- 作为选择符使用，在字段 a 中截取长度为 2 的子串并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$rightCP: 2}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "四五"
    }
    Return 1 row(s).
    ```

- 与匹配符配合使用，匹配字段 a 截取子串后值为“四五”的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$rightCP: 2, $et: "四五"}})
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




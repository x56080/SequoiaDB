##语法##

```lang-json
{<字段名>: {$rightBytes: <值>}}
```

##说明##

$rightBytes 用于从字符串末尾截取指定字节数的子串。当字段类型为字符串时，将按 UTF-8 编码规则对字符串进行截取；当字段类型为数组时，将对每个数组元素进行截取；当字段类型为非字符串时，将返回 null。

{$rightBytes: <值>} 中 <值> 为正整数，表示字节数。如果取值为 0 或负整数，将返回空串。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: "abcdefg"})
> db.sample.employee.insert({a: "12三四五"})
```

- 作为选择符使用，在字段 a 中截取字节数为 3 的子串并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$rightBytes: 3}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "efg"
    }
    {
        "_id": {
          "$oid": "640fd5973cd19cb20c6cebc9"
        },
        "a": "?五"
    }
    Return 2 row(s).
    ```

    > **Note：**
    >
    > 如果指定的字节数小于字符的字节数，将返回乱码。

- 与匹配符配合使用，匹配字段 a 截取子串后值为“efg”的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$rightBytes: 3, $et: "efg"}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "abcdefg"
    }
    Return 1 row(s).
  ```



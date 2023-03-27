
##语法##

```lang-json
{<字段名>: {$substrBytes: <值>}}
{<字段名>: {$substrBytes:[<起点>, <长度>]}})
```

##说明##

$substrBytes 用于截取指定字节数的子串。当字段类型为字符串时，将按 UTF-8 编码规则对字符串进行截取；当字段类型为数组时，将对每个数组元素进行截取；当字段类型为非字符串时，将返回 null。

- {$substrBytes: <值>}表示截取指定字节数的子串，其中 <值> 的取值如下：

    - 正整数：从字符串开头截取指定字节数的子串。
    - 0：返回空串。
    - 负整数：从字符串末尾第 N 个字节开始，截取所有字节。如果指定的值大于字符串的总字节数，将返回空串。

- {$substrBytes:[<起点>, <长度>]}表示从指定位置截取指定字节数的子串，其中：

    <起点> 的取值如下：

    - 自然数：以字符串开头第 N+1 个字节为起点，例如字符串为“12三四五”，<起点> 的取值为 5，表示以字符“四”的第一个字节为起点。
    - 负整数：以字符串末尾第 N 个字节为起点，例如字符串为“12三四五”，<起点> 的取值为 -3，表示以字符“五”的第一个字节为起点。

    <长度> 的取值如下：

    - 正整数：从 <起点> 开始，截取指定字节数的子串。
    - 0：返回空串。
    - 负整数：从 <起点> 开始，截取所有字节。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: "abcdefg"})
> db.sample.employee.insert({a: "12三四五"})
```

- 作为选择符使用

    以字符串开头为起点，在字段 a 中截取字节数为 3 的子串并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substrBytes: 3}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "abc"
    }
    {
        "_id": {
          "$oid": "640fd5973cd19cb20c6cebc9"
        },
        "a": "12?"
    }
    Return 2 row(s).
    ```

    > **Note：**
    >
    > 如果指定的字节数小于字符的字节数，将返回乱码。

    以字符串末尾第 3 个字符为起点，在字段 a 中截取所有字节并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substrBytes: -3}})
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
        "a": "五"
    }
    Return 2 row(s).
    ```

    以字符串开头第 3 个字节为起点，在字段 a 中截取字节数为 3 的子串并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substrBytes: [2, 3]}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "cde"
    }
    {
        "_id": {
          "$oid": "640fd5973cd19cb20c6cebc9"
        },
        "a": "三"
    }
    Return 2 row(s).
    ```

    以字符串开头第 3 个字节为起点，在字段 a 中截取所有字节并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substr: [2, -1]}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "cdefg"
    }
    {
        "_id": {
          "$oid": "640fd5973cd19cb20c6cebc9"
        },
        "a": "三四五"
    }
    Return 2 row(s).
    ```

    以字符串末尾第 3 个字节为起点，在字段 a 中截取字节数为 3 的子串并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substrBytes: [-3, 3]}})
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
        "a": "五"
    }
    Return 2 row(s).
    ```

- 配合匹配符使用，匹配字段 a 截取子串后值为“cde”的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$substrBytes: [2, 3], $et: "cde"}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "abcdefg"
    }
    Return 1 row(s).
    ```



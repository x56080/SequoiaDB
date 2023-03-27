
##语法##

```lang-json
{<字段名>: {$substrCP: <值>}}
{<字段名>: {$substrCP:[<起点>, <长度>]}})
```

##说明##

$substrCP 用于截取指定长度的子串，即截取指定[代码点][codePoint]数的子串。当字段类型为字符串时，将按 UTF-8 编码规则对字符串进行截取；当字段类型为数组时，将对每个数组元素进行截取；当字段类型为非字符串时，将返回 null。

- {$substrCP: <值>}表示截取指定长度的子串，<值> 的取值如下：

    - 正整数：从字符串开头截取指定长度的子串。
    - 0：返回空串。
    - 负整数：从字符串末尾第 N 个字符开始，截取所有字符。如果指定的值大于字符串的长度，将返回空串。

- {$substrCP:[<起点>, <长度>]}表示从指定位置截取指定长度的子串，其中：

    <起点> 的取值如下：

    - 自然数：以字符串开头第 N+1 个字符为起点，例如字符串为“abcde”，<起点> 的取值为 2，表示以字符 c 为起点。
    - 负整数：以字符串末尾第 N 个字符为起点，例如字符串为“abcde”，<起点> 的取值为 -2，表示以字符 d 为起点。

    <长度> 的取值如下：

    - 正整数：从 <起点> 开始，截取指定长度的子串。
    - 0：返回空串。
    - 负整数：从 <起点> 开始，截取所有字符。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: "12三四五"})
```

- 作为选择符使用

    以字符串开头为起点，在字段 a 中截取长度为 3 的子串并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substrCP: 3}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "12三"
    }
    Return 1 row(s).
    ```

    以字符串末尾第 2 个字符开始，在字段 a 中截取所有字符并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substrCP: -2}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "四五"
    }
    Return 1 row(s).
    ```

    以字符串开头第 3 个字符为起点，在字段 a 中截取长度为 3 的子串并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substrCP: [2, 3]}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "三四五"
    }
    Return 1 row(s).
    ```

    以字符串开头第 3 个字符为起点，在字段 a 中截取所有字符并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substr: [2, -1]}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "三四五"
    }
    Return 1 row(s).
    ```

    以字符串末尾第 2 个字符为起点，在字段 a 中截取长度为 1 的子串并返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$substrCP: [-2, 1]}})
    {
        "_id": {
          "$oid": "58257afbec5c9b3b7e000002"
        },
        "a": "四"
    }
    Return 1 row(s).
    ```

- 配合匹配符使用，匹配字段 a 截取子串后值为“三四五”的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$substrCP: [2, 3], $et: "三四五"}})
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


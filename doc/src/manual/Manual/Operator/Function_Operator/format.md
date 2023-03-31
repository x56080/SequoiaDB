##语法##

```lang-json
{<字段名>: {$format: <值>}}
```

##说明##

$format 用于将字段值舍入至指定小数位，并按照"#,###.##"进行格式化。当字段类型为数字时，将直接对字段值进行舍入并格式化；当字段类型为数组时，将对每个数组元素进行舍入并格式化；当字段类型为字符串或布尔时，会先将字段值转换为数值，再对数值进行舍入并格式化；其余类型的字段值暂不支持转换，将返回 null。

{$format: <值>} 中 <值> 的取值如下：

- 正整数：四舍五入至指定的小数位并进行格式化。如果指定的位数大于小数位数，将自动用 0 补齐。
- 0或负整数：四舍五入为整数并进行格式化。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: 1231.445})
> db.sample.employee.insert({a: true})
> db.sample.employee.insert({a: "2aa"})
> db.sample.employee.insert({a: "aa123bb"})
```

- 作为选择符使用，将字段 a 四舍五入至一位小数并格式化后返回

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$format: 1}})
    {
        "_id": {
          "$oid": "641428ab5e1cb173b9d1d40a"
        },
        "a": "1,231.4"
    }
    {
        "_id": {
          "$oid": "641428ab5e1cb173b9d1d40b"
        },
        "a": "1.0"
    }
    {
        "_id": {
          "$oid": "642254ea6323a85a2d4506f6"
        },
        "a": "2.0"
    }
    {
        "_id": {
          "$oid": "64225e716323a85a2d4506f8"
        },
        "a": "0.0"
    }
    Return 4 row(s).
    ```

    > **Note:**
    >
    > - 当字段类型为布尔时，如果字段值为 true，将转换为 1 并进行格式化；如果字段值为 false，将转换为 0 并进行格式化。
    > - 当字段值为字符串时，如果字符串以数字开头，将转换数字部分并进行格式化，否则转换为 0 并进行格式化。

- 与匹配符配合使用，匹配字段 a 舍入为整数并格式化后值为"1,231"的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$format: -1, $et: "1,231"}})
    {
        "_id": {
          "$oid": "641428ab5e1cb173b9d1d40a"
        },
        "a": 1231.445
    }
    Return 1 row(s).
    ```


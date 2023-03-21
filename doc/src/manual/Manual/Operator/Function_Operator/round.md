
##语法##

```lang-json
{<字段名>: {$round: <值>}}
```

##说明##

$round 用于将字段值舍入为整数或指定的小数位。当字段类型为数组时，将对所有数组元素执行该操作；当字段类型为非数字时，将返回 null。

{$round: <值>} 中 <值> 的可选取值如下：

- 正整数：四舍五入到指定的小数位，例如字段 a 的取值为 123.456，则`{a: {$round: 1}}`的结果为 123.5。
- 0：四舍五入为整数，例如字段 a 的取值为 123.456，则`{a: {$round: 0}}`的结果为 123。
- 负整数：对指定的整数位进行四舍五入并返回整数，例如字段 a 的取值为 123.456，则`{a: {$round: -2}}`的结果为 100。如果指定的位数大于整数位数将返回 0，例如字段 a 的取值为 123.456，则`{a: {$round: -5}}`的结果为 0。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: 31.445})
```

- 作为选择符使用，返回字段 a 四舍五入到一位小数的结果

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$round: 1}})
    {
        "_id": {
          "$oid": "6401acfe4825f7b918caee04"
        },
        "a": 31.4
    }
    Return 1 row(s).
    ```

- 与匹配符配合使用，匹配字段 a 四舍五入后值为 30 的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$round: -1, $et: 30}})
    {
        "_id": {
          "$oid": "6401acfe4825f7b918caee04"
        },
        "a": 31.445
    }
    Return 1 row(s).
    ```


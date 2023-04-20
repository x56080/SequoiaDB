
##语法##

```lang-json
{<字段名>: {$ifnull: <值>}}
```

##说明##

$ifnull 用于判断指定字段是否不存在、字段值是否为 null 或字段类型是否为 Undefined。如果字段满足上述任意一种情况，将返回表达式 {$ifnull: <值>} 中指定的值；如果不满足上述情况，则直接返回该字段的值。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: 1})
> db.sample.employee.insert({a: null})
```

- 作为选择符使用，返回字段 a 经过判断、处理后的结果

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$ifnull: "isnull"}})
    {
        "_id": {
          "$oid": "643a4ccfb4e498a52ec474a9"
        },
        "a": 1
    {
        "_id": {
          "$oid": "643a4cd2b4e498a52ec474aa"
        },
        "a": "isnull"
    }
    Return 2 row(s).
    ```

- 与匹配符配合使用，匹配字段 a 不存在、字段值为 null 或字段类型为 Undefined 的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$ifnull: "isnull", $et: "isnull"}})
    {
        "_id": {
          "$oid": "643a4cd2b4e498a52ec474aa"
        },
        "a": null
    }
    Return 1 row(s).
    ```


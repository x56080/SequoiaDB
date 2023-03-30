
##语法##

```lang-json
{<字段名>: {$multiply: <值>}}
```

##说明##

$multiply 用于对字段值进行乘法运算。当字段类型为数组时，将对所有数组元素进行乘法运算；当字段类型为非数字时，将返回 null。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: 3})
```

- 作为选择符使用，返回字段 a 乘以 10 的结果

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$multiply: 10}})
    {
        "_id": {
          "$oid": "58256a7b2b4c38286d000020"
        },
        "a": 30
    }
    Return 1 row(s).
    ```

- 与匹配符配合使用，匹配字段 a 与 10 相乘后值为 30 的记录
  
    ```lang-javascript
    > db.sample.employee.find({a: {$multiply: 10, $et: 30}})
    {
        "_id": {
          "$oid": "58256a7b2b4c38286d000020"
        },
        "a": 3
    }
    Return 1 row(s).
    ```
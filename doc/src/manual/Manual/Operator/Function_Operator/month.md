##语法##

```lang-json
{<字段名>: {$month: 1}}
```

##说明##

$month 用于获取日期时间中的月份。当字段类型为数组时，将对每个数组元素进行处理；当字段类型为 Double 或 Decimal 时，会先将字段类型转换为 Int64，再对转换后的值进行处理；当字段类型为 String 时，仅对"YYYY-MM-DD"格式的日期字符串进行处理；当字段类型无法处理时，将返回 null。

SequoiaDB 支持处理的类型包括 Int32、Int64、Double、Decimal、String、Date 和 Timestamp。当字段类型为 Int32 时，该字段值表示绝对秒数；当字段类型为 Int64、Double 或 Decimal 时，该字段值表示绝对毫秒。

> **Note:**
>
> - 绝对秒数：距离格林威治时间1970年01月01日00时00分00秒的总秒数。
> - 绝对毫秒：距离格林威治时间1970年01月01日00时00分00秒的总毫秒数。

##示例##

在集合 sample.employee 插入如下记录：

```lang-javascript
> db.sample.employee.insert({a: {$date: "2012-01-01"}})
> db.sample.employee.insert({a: [{$date: "2013-01-01"}, "2014-02-03"]})
> db.sample.employee.insert({a: "2015-03-04"})
> db.sample.employee.insert({a: {$decimal: "1.2E+10"}})
> db.sample.employee.insert({a: {$decimal: "1.7E+300"}})
> db.sample.employee.insert({a: 1.7E+300})
```

- 作为选择符使用，返回字段 a 日期时间中的月份

    ```lang-javascript
    > db.sample.employee.find({}, {a: {$month: 1}})
    {
      "_id": {
        "$oid": "58257afbec5c9b3b7e000002"
      },
      "a": 1
    }
    {
      "_id": {
        "$oid": "641d4fa0ff0a7db08dd5ee29"
      },
      "a": [
        1,
        2
      ]
    }
    {
      "_id": {
        "$oid": "6433c47609e0029e1e0d60ed"
      },
      "a": 3
    }
    {
      "_id": {
        "$oid": "6433b79e73a298d4f50f5abf"
      },
      "a": 5
    }
    {
      "_id": {
        "$oid": "6433b2f5df28ca4bfad27d06"
      },
      "a": null
    }
    {
      "_id": {
        "$oid": "6433b30adf28ca4bfad27d07"
      },
      "a": 5
    }
    Return 6 row(s).
    ```

    > **Note:**
    >
    > - {$month: 1} 中 1 没有特殊含义，仅作为占位符出现。
    > - 当 Decimal 类型的值超出 Int64 类型的取值范围时，将异常返回 null。
    > - 将 Double 或 Decimal 类型转换为 Int64 类型时，可能会出现精度丢失。

- 与匹配符配合使用，匹配字段 a 的月份为 2 的记录

    ```lang-javascript
    > db.sample.employee.find({a: {$month: 1, $et: 2}})
    {
      "_id": {
        "$oid": "641d4fa0ff0a7db08dd5ee29"
      },
      "a": [
        {
          "$date": "2013-01-01"
        },
        "2014-02-03"
      ]
    }
    Return 1 row(s).
    ```
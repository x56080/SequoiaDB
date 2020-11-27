##描述##

序列快照 SDB_SNAP_SEQUENCES 列出当前数据库中所有序列的属性信息，每个序列一条记录。

##标示##

SDB_SNAP_SEQUENCES

>   **Note:**
>
>   只能在协调节点执行。

##字段信息##

| 字段名              | 类型   | 描述                         |
| ------------------- | ------ | ---------------------------- |
| Name                | 字符串 | 序列名称                     |
| ID                  | 长整型 | 序列ID                       |
| Increment           | 整型   | 序列增加的间隔               |
| StartValue          | 长整型 | 序列起始值                   |
| CurrentValue        | 长整型 | 序列当前值                   |
| MinValue            | 长整型 | 序列最小值                   |
| MaxValue            | 长整型 | 序列最大值                   |
| CacheSize           | 整型   | 编目节点每次缓存序列值数     |
| AcquireSize         | 整型   | 协调节点每次获取序列值数     |
| Cycled              | 布尔   | 序列值到达最大值（或最小值）是否允许循环 |
| Version             | 整型   | 序列版本号                   |
| Initial             | 布尔   | 该序列是否未使用，true表示未使用 |
| Internal            | 布尔   | 该序列是否是系统内部序列     |

> **Note:**
>
> 序列未使用时，序列不存在当前值（CurrentValue）。使用前，CurrentValue 会显示为 StartValue，但该值无意义。

##示例##

```lang-javascript
> db.snapshot( SDB_SNAP_SEQUENCES )
{
  "AcquireSize": 1000,
  "CacheSize": 1000,
  "CurrentValue": 5000,
  "Cycled": false,
  "ID": 4,
  "Increment": 10,
  "Initial": true,
  "Internal": true,
  "MaxValue": {
    "$numberLong": "9223372036854775807"
  },
  "MinValue": 1,
  "Name": "SYS_21333102559237_studentID_SEQ",
  "StartValue": 5000,
  "Version": 1,
  "_id": {
    "$oid": "5bd8fcfc8af29ca6ad2a32e8"
  }
}
Return 1 row(s).
Takes 0.009799s.
```

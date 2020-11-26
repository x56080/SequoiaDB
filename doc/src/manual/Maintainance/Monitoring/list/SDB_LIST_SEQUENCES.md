##描述##

序列列表 SDB_LIST_SEQUENCES 列出当前数据库的全部序列信息。

> **Note:** 只支持coord节点上使用。

##标示##

SDB_LIST_SEQUENCES

##字段信息##

| 字段名         | 类型   | 描述                        |
| -------------- | ------ | --------------------------- |
| Name           | 字符串 | 序列名                      |

##示例##

```lang-javascript
> db.list(SDB_LIST_SEQUENCES)
{
  "Name": "SYS_21333102559237_studentID_SEQ"
}
{
  "Name": "SYS_21333102559238_teacherID_SEQ"
}
...
```

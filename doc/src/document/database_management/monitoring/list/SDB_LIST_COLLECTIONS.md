##描述##

集合列表 SDB_LIST_COLLECTIONS 列出当前数据库节点中所有的非临时集合（协调节点中列出用户集合），每个集合为一条记录。

##标示##

SDB_LIST_COLLECTIONS

##字段信息##

| 字段名 | 类型   | 描述       |
| ------ | ------ | ---------- |
| Name   | 字符串 | 集合完整名 |

##示例##

```lang-javascript
> db.list( SDB_LIST_COLLECTIONS )
{
  "Name": "foo.bar"
}
...
```

##描述##

集合空间列表 SDB_LIST_COLLECTIONSPACES 列出当前数据库节点中所有的集合空间，每个集合空间为一条记录。

##标示##

SDB_LIST_COLLECTIONSPACES

##字段信息##

| 字段名 | 类型   | 描述       |
| ------ | ------ | ---------- |
| Name   | 字符串 | 集合空间名 |

##示例##

```lang-javascript
> db.list( SDB_LIST_COLLECTIONSPACES )
{
  "Name": "foo"
}
...
```

##描述##

序列列表 $LIST_SEQUENCES 列出当前数据库的全部序列信息。

> **Note:** 只支持coord节点上使用。

##标示##

$LIST_SEQUENCES

##字段信息##

| 字段名         | 类型   | 描述                        |
| -------------- | ------ | --------------------------- |
| Name           | 字符串 | 序列名                      |

##示例##

```lang-javascript
> db.exec( "select * from $LIST_SEQUENCES" )
{
  "Name": "SYS_2469606195201_studentID_SEQ"
}
```

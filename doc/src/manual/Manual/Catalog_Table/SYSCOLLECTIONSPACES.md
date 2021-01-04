
SYSCAT.SYSCOLLECTIONSPACES 集合中包含了该集群中所有的用户集合空间信息，每个用户集合空间保存为一个文档。

每个文档包含以下字段：

|  字段名     |   类型    |    描述                                  |
|-------------|-----------|------------------------------------------|
|  Name       |   string  | 集合空间名                               |
|  Collection |   array   | 该集合空间中包含的所有的集合名，每个集合表示为一个嵌套的 JSON 对象，包含 Name 字段与相应的集合名 |
|  Group      |   array   | 该集合空间所在的复制组 ID              |
|  PageSize   |   number  | 该集合空间的数据页大小，单位为字节   |
| LogPageSize |   number  | 该集合空间的大对象 LOB 数据页大小，单位为字节 |

##示例##

包含一个集合且存放在某一复制组中的集合空间信息如下：

```lang-json
{
  "Collection" : [ { "Name" : "sample" } ],
  "Group" : [ { "GroupID" : 1000 } ],
  "Name" : "test",
  "PageSize" : 4096
}
```


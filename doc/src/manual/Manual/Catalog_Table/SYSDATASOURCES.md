## 所属集合空间

SYSCAT

## 概念

SYSCAT.SYSDATASOURCES 集合中包含了该集群中所有的数据源的元数据信息。每个数据源保存为一个文档。

每个文档包含以下字段：

| 字段名 | 类型 | 描述 |
| ------ | ---- | ---- |
| ID     | number | 数据源 ID，从 1 开始递增 |
| Name   | string | 数据源名称  |
| Version | number | 数据源的元数据版本号，从 0 开始递增，当数据源信息发生变化时改变 |
| Type   | string | 数据源类型，当前仅支持 SequoiaDB |
| DSVersion | string | 数据源软件版本号，从添加的数据源中获取 |
| Address | string | 数据源服务地址列表，即作为数据源的 SequoiaDB 集群中所有或部分协调节点的地址，每个地址的格式为\<hostname:svcname\>或\<IP:svcname\> |
| User | string | 数据源用户名 |
| Password | string | 数据源用户对应的密码 |
| ErrorControlLevel | string | 对使用了数据源的集合或集合空间进行不支持的数据操作（如 DDL）时的报错级别 |
| AccessModeDesc | string | 数据源的访问权限掩码描述，取值如下：<br> "READ"：允许进行只读操作 <br>  "WRITE"：允许进行写操作 <br>  "READ\|WRITE"：允许进行所有操作  <br>  "NONE"：不允许进行任何操作 |
| AccessMode | number | 数据源的访问权限掩码，取值如下：<br> 1：对应"READ"，允许进行只读操作 <br>  2：对应"WRITE"，允许进行写操作<br>  3：对应"ALL"或"READ\|WRITE"，允许进行所有操作  <br>  0：对应"NONE"，不允许进行任何操作 |
| ErrorFilterMaskDesc | string | 数据源的错误过滤掩码描述，取值如下：<br> 	"READ"：过滤数据读错误  <br>   	"WRITE"：过滤数据写错误  <br> 	"READ\|WRITE"：过滤所有数据读写错误   <br>  "NONE"：不对任何错误进行过滤 |
| ErrorFilterMask | number | 数据源的错误过滤掩码，取值如下：<br> 	1：对应"READ"，过滤数据读错误  <br>   	2：对应"WRITE"，过滤数据写错误  <br> 	3：对应"ALL"或"READ\|WRITE"，过滤所有数据读写错误   <br>  0：对应"NONE"，不对任何错误进行过滤 |


## 示例

一个典型的数据源元数据信息如下：

```lang-json
{
  "_id": {
    "$oid": "5ffc365c72e60c4d9be30c50"
  },
  "ID": 2,
  "Name": "datasource",
  "Type": "SequoiaDB",
  "Version": 0,
  "DSVersion": "3.4.1",
  "Address": "sdbserver:11810",
  "User": "sdbadmin",
  "Password": "d41d8cd98f00b204e9800998ecf8427e",
  "ErrorControlLevel": "High",
  "AccessMode": 1,
  "AccessModeDesc": "READ",
  "ErrorFilterMask": 0
  "ErrorFilterMaskDesc": "NONE"
}
```
##语法##
***rg.getDetail()***

获取当前分区组信息。

##返回值##

返回当前分区组详细信息，类型为 json 对象。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

获取 group1 分区组的详细信息，该分区组存在3个节点

```lang-javascript
> var rg = db.getRG("group1")
> rg.getDetail()
{
  "Group": [
    {
      "HostName": "hostname1",
      "Status": 1,
      "dbpath": "/opt/sequoiadb/database/data/11830/",
      "Service": [
        {
          "Type": 0,
          "Name": "11830"
        },
        {
          "Type": 1,
          "Name": "11831"
        },
        {
          "Type": 2,
          "Name": "11832"
        }
      ],
      "NodeID": 1002
    },
    {
      "HostName": "hostname2",
      "Status": 1,
      "dbpath": "/opt/sequoiadb/database/data/11840/",
      "Service": [
        {
          "Type": 0,
          "Name": "11840"
        },
        {
          "Type": 1,
          "Name": "11841"
        },
        {
          "Type": 2,
          "Name": "11842"
        }
      ],
      "NodeID": 1004
    },
    {
      "HostName": "hostname3",
      "Status": 1,
      "dbpath": "/opt/sequoiadb/database/data/11850/",
      "Service": [
        {
          "Type": 0,
          "Name": "11850"
        },
        {
          "Type": 1,
          "Name": "11851"
        },
        {
          "Type": 2,
          "Name": "11852"
        }
      ],
      "NodeID": 1006
    }
  ],
  "GroupID": 1002,
  "GroupName": "group1",
  "PrimaryNode": 1004,
  "Role": 0,
  "Status": 1,
  "Version": 7,
  "_id": {
    "$oid": "580043577e70618777a2cf39"
  }
}
```
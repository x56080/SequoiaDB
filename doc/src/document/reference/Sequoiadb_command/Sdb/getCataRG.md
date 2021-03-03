##名称##

getCataRG - 获取编目分区组的引用

##语法##

***db.getCataRG()***

##类别##

Sdb

##描述##

该函数用于获取编目分区组的引用，用户可以通过该引用对分区组进行相关操作。

##参数##

无

##返回值##

函数执行成功时，返回 SdbReplicaGroup 类型的对象。

函数执行失败时，将抛出异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。


##示例##

获取编目分区组引用

```lang-javascript
> var rg = db.getCataRG()
```

通过该引用获取分区组的详细信息

```lang-javascript
> rg.getDetail()
{
  "Group": [
    {
      "dbpath": "/opt/sequoiadb/database/catalog/11800",
      "HostName": "u1604-lxy",
      "Service": [
        {
          "Type": 0,
          "Name": "11800"
        },
        {
          "Type": 1,
          "Name": "11801"
        },
        {
          "Type": 2,
          "Name": "11802"
        },
        {
          "Type": 3,
          "Name": "11803"
        }
      ],
      "NodeID": 1,
      "Status": 1
    }
  ],
  "GroupID": 1,
  "GroupName": "SYSCatalogGroup",
  "PrimaryNode": 1,
  "Role": 2,
  "SecretID": 1519705199,
  "Status": 1,
  "Version": 1,
  "_id": {
    "$oid": "5ff52af05c0657cec2f706d5"
  }
}
```

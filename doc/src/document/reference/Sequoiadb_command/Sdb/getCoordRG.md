##名称##

getCoordRG - 获取协调分区组的引用

##语法##

**db.getCoordRG()**

##类别##

Sdb

##描述##

该函数用于获取协调分区组的引用，用户可以通过该引用对分区组进行相关操作。

##参数##

无

##返回值##

函数执行成功时，返回 SdbReplicaGroup 类型的对象。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##版本##

v2.0 及以上版本

##示例##

获取协调分区组的引用

```lang-javascript
> var rg = db.getCoordRG()
```

通过该引用获取分区组的详细信息

```lang-javascript
> rg.getDetail()
{
  "Group": [
    {
      "HostName": "sdbserver",
      "Status": 1,
      "dbpath": "/opt/sequoiadb/database/coord/11810/",
      "Service": [
        {
          "Type": 0,
          "Name": "11810"
        },
        {
          "Type": 1,
          "Name": "11811"
        },
        {
          "Type": 2,
          "Name": "11812"
        }
      ],
      "NodeID": 2
    }
  ],
  "GroupID": 2,
  "GroupName": "SYSCoord",
  "Role": 1,
  "SecretID": 112493285,
  "Status": 1,
  "Version": 2,
  "_id": {
    "$oid": "5ff52af65c0657cec2f706d9"
  }
}
```
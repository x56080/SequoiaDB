##语法##
***backupOffline( [options] )***

备份数据库。

##参数描述##

| 参数名 |参数类型| 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| options |Json 对象| 设定备份名，指定复制组，备份方式等参数 | 否 |

 1. **options 格式**

 | 属性名 | 描述 | 格式 |
 | ------ | ------ | ------ |
 | GroupID | 指定备份的复制组 ID，缺省为所有复制组 | GroupID: 1000 或 GroupID: [1000, 1001] |
 | GroupName | 指定备份的复制组名，缺省为所有复制组 | GroupName: "data1" 或 GroupName: ["data1", "data2"] |
 | Name | 备份名称，缺省为 “YYYY-MM-DD-HH:mm:SS” 时间格式的备份名 |  Name: "backup-2014-1-1" |
 | Path | 备份路径，缺省为配置参数指定的备份路径。该路径支持通配符（%g/%G: group name, %h/%H: host name, %s/%S: service name）| Path: "/opt/sequoiadb/backup/%g" |
 | IsSubDir | 上述 Path 参数所配置的路径是否为配置参数指定的备份路径的子目录，缺省为 false | IsSubDir: false |
 | Prefix | 备份前缀名，支持通配符（%g,%G,%h,%H,%s,%S），缺省为空 | Prefix: "%g_bk_" |
 | EnableDateDir | 是否开启日期子目录功能，如果开启则会自动根据当前日期创建 “YYYY-MM-DD” 的子目录，缺省为 false | EnableDateDir: false |
 | Description | 备份描述 | Description: "First backup" |
 | EnsureInc | 是否开启增量备份，缺省为 false | EnsureInc: false |
 | OverWrite | 存在同名备份是否覆盖，缺省为 false | OverWrite: false |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

**对整个数据库进行全量备份**

```lang-javascript
> db.backupOffline( { Name: "FullBackup1" } )
> db.listBackup()
{
  "Name": "FullBackup1",
  "NodeName": "susetzb:30000",
  "GroupName": "SYSCatalogGroup",
  "EnsureInc": false,
  "BeginLSNOffset": 0,
  "EndLSNOffset": 5299104,
  "StartTime": "2015-10-20-16:52:42",
  "HasError": false
}
{
  "Name": "FullBackup1",
  "NodeName": "susetzb:40000",
  "GroupName": "db2",
  "EnsureInc": false,
  "BeginLSNOffset": 0,
  "EndLSNOffset": 230209508,
  "StartTime": "2015-10-20-16:52:42",
  "HasError": false
}
{
  "Name": "FullBackup1",
  "NodeName": "susetzb:20000",
  "GroupName": "db1",
  "EnsureInc": false,
  "BeginLSNOffset": 0,
  "EndLSNOffset": 272453160,
  "StartTime": "2015-10-20-16:52:42",
  "HasError": false
}
Return 3 row(s).
```

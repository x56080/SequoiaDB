
##名称##

backup - 备份数据库。

##语法##

**db.backup([options])**

##类别##

Sdb

##描述##

备份数据库。

##参数描述##

* `options` ( *Object*， *选填* )

   	`options`参数可以设置备份的属性，如指定设定备份名，指定复制组，备份方式等。
	可组合使用 `options` 的如下选项：

    1. `GroupID` ( *Array* )：指定备份的复制组 ID，缺省为所有复制组。

        格式：`GroupID:1000` 或 `GroupID:[1000, 1001]`

    2. `GroupName` ( *String* )：指定备份的复制组名，缺省为所有复制组。

        格式：`GroupName: "data1"` 或 `GroupName: ["data1", "data2"]`

    3. `Name` ( *String* )：备份名称，缺省为 “YYYY-MM-DD-HH:mm:SS” 时间格式的备份名。

		格式：`Name: "backup-2014-1-1"`

    4. `Path` ( *String* )：备份路径，缺省为配置参数指定的备份路径。 该路径支持通配符 （%g/%G: group name, %h/%H: host name, %s/%S: service name）。 当在协调节点上执行命令使用该参数时，需要使用通配符，以避免所有的节点往同一个路径下进行操作而导致未知IO错误。

		格式：`Path: "/opt/sequoiadb/backup/%g"`

    5. `IsSubDir` ( *Bool* )：上述 `Path` 参数所配置的路径是否为配置参数指定的备份路径的子目录，如果为true，则真实的备份目录为：" 配置参数中指定的备份目录 / `Path`目录 "。 缺省为 false。

		格式：`IsSubDir: false`

    6. `Prefix` ( *String* )：备份前缀名，支持通配符（%g,%G,%h,%H,%s,%S），缺省为空。	

		格式：`Prefix: "%g_bk_"`

    7. `EnableDateDir` ( *Bool* )：是否开启日期子目录功能，如果开启则会自动根据当前日期创建 “YYYY-MM-DD” 的子目录，缺省为 false。

		格式：`EnableDateDir: false`

    8. `Description` ( *String* )：备份描述。

		格式：`Description: "First backup"`

    9. `EnsureInc` ( *Bool* )：是否开启增量备份，缺省为 false。

		格式：`EnsureInc: false`

    10. `OverWrite` ( *Bool* )：存在同名备份是否覆盖，缺省为 false。

		格式：`OverWrite: false`

    11. `Compressed` ( *Bool* )：是否开启数据压缩，缺省为 true。

        格式：`Compressed: true`

    12. `CompressionType` ( *String* )：压缩格式类型，取值"lz4"、"snappy"和"zlib"，缺省为 "snappy"。

        格式：`CompressionType: "zlib" `

    13. `BackupLog` ( *Bool* )：当全量备份时是否需要备份所有日志，缺省为 false。

        格式：`BackupLog: false`

##返回值##

成功：返回新集合的对象。  

失败：抛出异常。

##错误##

`backup()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ----------------------- | --- | ------ |
| -240   | SDB_BAR_BACKUP_EXIST    | 相同名字的备份已存在 | 先删除该备份或开启 `OverWrite: true` |
| -241   | SDB_BAR_BACKUP_NOTEXIST | 增量备份对应的全量备份不存在 | 先执行一次全量备份 |
| -70    | SDB_BAR_DAMAGED_BK_FILE | 备份文件已损坏 | - |
| -57    | SDB_DPS_LOG_NOT_IN_BUF  | 增量备份的开始日志已不存在 | 重新执行全量备份后再增量备份 |
| -98    | SDB_DPS_CORRUPTED_LOG   | 相同日志Hash校验不一致，日志发生变更 | 重新执行全量备份后再增量备份 |

当异常抛出时，可以通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取[错误码](manual/Manual/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/faq.md)了解更多内容。

##版本##

v1.2及以上版本。  
v2.8.2及以上版本增加 `Compressed`、`CompressionType` 和 `BackupLog` 参数。

##示例##

1. 对数据库节点进行全量备份。

	```lang-javascript
	> db.backup( { Name: "FullBackup1" } )
	> db.listBackup()
	{
		"Version": 2,
	  	"Name": "FullBackup1",
		"ID": 0,
	  	"NodeName": "susetzb:30000",
    		"GroupName": "SYSCatalogGroup",
    		"EnsureInc": false,
	  	"BeginLSNOffset": 0,
    		"EndLSNOffset": 195652068,
    		"StartTime": "2015-10-20-16:52:42",
		"LastLSN": 195652020,
		"LastLSNCode": 1845751176,
    		"HasError": false
	}
	Return 1 row(s).
	```

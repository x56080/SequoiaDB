##NAME##

listBackup - Enumerate database backup

##SYNOPSIS##

**db.listBackup( [options], [cond], [sel], [sort] )**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enumerate the backups in the current database.

##PARAMETERS##

| Name    | Type   | Description    | Required or Not |
|---------|--------|----------------|-----------------|
| options | Json   |Specify the backup name,replication group,path and other parameters.     | Not  |
| cond    | Json   |Backup filter conditions.       |Not       |
| sel     | Json   |Select the field for backup output. | Not       |
| sort    | Json   |Sort the returned records by the selected field. 1 is ascending and -1 is descending.       | Not 	   |

###Options format###

| Name     	| Description    					| format 									|
|-----------|---------------------------------------|---------------------------------------|
| GroupID 	| Specify the backup copy group ID,the default is all copy group. | GroupID:1000 or GroupID:[1000, 1001]  |
| GroupName | Specify the name of backup copy group,the default is all the copy groups. 	| GroupName:"data1" or GroupName:["data1", "data2"] |
| Name 		| Specify the backup name,the default is all backups.		| Name:"backup-2014-1-1" 				|
| Path 		| Specify the backup path.The default is the backup path specified by the configuration parameter.The path supports wildcards（%g/%G: group name, %h/%H: host name, %s/%S:service name）.When using this parameter to execute commands on the coordination node,wildcards are required to avoid all nodes operating under the same path and causing unknow IO errors.| Path:"/opt/sequoiadb/backup/%g" |
| IsSubDir 	| Whether the path configured by the above Path parameter is the subdirectory of the backup path specified by the configuration  prarameter.if it is true,the real backup specified by the configuration parameter,if true,the real real backup directory is : " The backup directory in the parameter / `Path`catalog ";Default is false. | IsSubDir:false |
| Prefix    | Backup prefix,Support wildcard(%g,%G,%h,%H,%s,%S),Default is null. | Prefix:"%g_bk_" |
| Detail    | Whether Show Details,Default is false.        | Detail: true |

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the cursor.Users can refer to [$LIST_BACKUP][LIST_BACKUP] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the error code. For more details, refer to [Troubleshooting][error_guide].

##VERSION##

v2.0 and above

##EXAMPLES##

* View all backup information under the backup path specified by the database configuration parameter

	```lang-javascript
	> db.listBackup()
	{
	  "Version": 2,
	  "Name": "test",
	  "ID": 0,
	  "NodeName": "vmsvr2-suse-x64-1:20000",
	  "GroupName": "db1",
	  "EnsureInc": false,
	  "BeginLSNOffset": 195652020,
	  "EndLSNOffset": 195652068,
	  "TransLSNOffset": -1,
	  "StartTime": "2017-06-20-13:02:22",
	  "LastLSN": 195652020,
	  "LastLSNCode": 1845751176,
	  "HasError": false
	}
	```

* View backup information in other paths

  Backup data nodes to other paths

    ```lang-javascript
    > var datadb = new Sdb( "localhost", 20000 )
    > datadb.backup( { Path: "/tmp/sequoiadb_backup/20000" } )
    ```

  Connect coord to view backup information and specify the Path parameter when listBackup,otherwise the backup information cannot be found in the default path.

    ```lang-javascript
    > var db = new Sdb( "localhost", 11810 ) 
    > db.listBackup( { Path: "/tmp/sequoiadb_backup/20000" } )
    {
      "Version": 2,
      "Name": "2017-10-26-10:14:11",
      "ID": 0,
      "NodeName": "ubuntu-test-03:20000",
      "GroupName": "db1",
      "EnsureInc": false,
      "BeginLSNOffset": -1,
      "EndLSNOffset": 375546828,
      "TransLSNOffset": -1,
      "StartTime": "2017-10-26-10:14:11",
      "LastLSN": -1,
      "LastLSNCode": 0,
      "HasError": false
    }


[^_^]:
     本文使用的所有引用及链接
[LIST_BACKUP]:manual/Manual/SQL_Grammar/Monitoring/LIST_BACKUP.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/faq.md
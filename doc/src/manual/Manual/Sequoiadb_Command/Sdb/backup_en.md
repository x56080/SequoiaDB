 
##NAME##

backup - backup database

##SYNOPSIS##

**db.backup([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to backup the entire database or specify the replication group.

##PARAMETERS##

options ( *object, optional* )

Set the attributes of the backup, such as backup name, specify the replication group to be backed up, backup method. The options that can be combined are as follows:

- GroupID ( *array* ): Specify the ID of the replication group to be backed up, the default is all replication groups.

   Format: `GroupID:1000` or `GroupID:[1000, 1001]`

- GroupName ( *string* ): Specify the name of the replication group to be backed up, the default is all replication groups.

   Format: `GroupName: "data1"` or `GroupName: ["data1", "data2"]`

- Name ( *string* ): Backup name, the default is the backup name in "YYYY-MM-DD-HH:mm:SS" time format.

   Format: `Name: "backup-2014-1-1"`

- Path ( *string* ): Backup path, the default is the backup path specified by node configuration item parameter [bkuppath][path].

   If the user needs to customize the backup path, wildcards can be used in the path parameter to allow each node to store the backup file in a different path to avoid all nodes operating under the same path and causing errors. The supported wildcards include %g/%G(for group name), %h/%H(for host name), and %s/%S(for service name).

   Format: `Path: "/opt/sequoiadb/backup/%g"`

- IsSubDir ( *boolean* ): Whether the path configured by the above path parameter is a subdirectory of the backup path specified by the configuration parameter, the default is false.

   If the parameter value is true, the real backup directory is `the backup derectory/Path directory` specified in the configuration parameters.

   Format: `IsSubDir: false`

- Prefix ( *string* ):  Backup prefix name, supports wildcard(%g, %G, %h, %H, %s, %S), the default is null.

    Format: `Prefix: "%g_bk_"`

- EnableDateDir ( *boolean* ): Whether to enable the date subdirectory function, the default is false.

   If the parameter value is true, a subdirectory named “YYYY-MM-DD” will be automatically created based on the current date.

   Format: `EnableDateDir: false`

- Description ( *string* ): Backup description.

   Format: `Description: "First backup"`

- EnsureInc ( *boolean* ): Whether to enable incremental backup, the default  is false.

   Format: `EnsureInc: false`

- OverWrite ( *boolean* ): Whether to overwrite a backup with the same name, the default is false.

   Format: `OverWrite: false`

- Compressed ( *boolean* ): Whether to enable data compression, the default is true.

   Format: `Compressed: true`

- CompressionType ( *string* ): Compression format type, the value includes "lz4", "snappy" and "zlib", the default is "snappy".

   Format: `CompressionType: "zlib" `

- BackupLog ( *boolean* ): Whether all logs need to be backed up when full backup, the default is false.

   Format: `BackupLog: false`

> **Note:**
>
> Added Compressed, CompressionType and BackupLog parameters for v2.8.2 and above.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.
 
##ERRORS##

The common exceptions of `backup()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -240   | SDB_BAR_BACKUP_EXIST    | A backup with the same name already exists. | Delete the backup first or set OverWrite to true.  |
| -241   | SDB_BAR_BACKUP_NOTEXIST | The full backup corresponding to the incremental backup does not exist. | Perform a full backup first. |
| -70    | SDB_BAR_DAMAGED_BK_FILE | The backup file is corrupted | - |
| -57    | SDB_DPS_LOG_NOT_IN_BUF  | The start log of the incremental backup no longer exists | Perform an incremental backup after re-executing a full backup. |
| -98    | SDB_DPS_CORRUPTED_LOG   | The hash check of the same log is inconsistent, and the log has changed | Perform an incremental backup after re-executing a full backup. |

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.2 and above

##EXAMPLES##

Perform a full backup of the replication group group1

```lang-javascript
> db.backup({Name:"backupName",Description:"backup group1",GroupName:"group1"})
```


[^_^]:
    links
[path]:manual/Manual/Database_Configuration/configuration_parameters.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/faq.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
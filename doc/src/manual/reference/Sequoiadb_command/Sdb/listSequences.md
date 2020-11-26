##语法##

***db.listSequences()***

##类别##

Sdb

##描述##

查看当前数据库的序列名称。

>**Note:**

>listSequences() 只会列出序列的名称。如果想要查看序列的详细信息，可以使用 [db.snapshot(SDB_SNAP_SEQUENCES)](manual/Maintainance/Monitoring/snapshot/SDB_SNAP_SEQUENCES.md)。

##参数##

无

##返回值##

返回当前数据库的序列名称。

##错误##

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 查看当前数据库的序列名称。

  ```lang-javascript
  > db.listSequences()
  {
      "Name": "SYS_8589934593_id_SEQ"
  }
  ```

##名称##

stopAllNodes - 在目标集群控制器（sdbcm）所在的机器中停止所有属于指定业务的节点。

##语法##

**oma.stopAllNodes(\[businessName\])**

##类别##

Oma

##描述##

在目标集群控制器（sdbcm）所在的机器中停止所有属于指定业务的节点。

**Note:**

* oma对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。

* 如果不指定业务名，默认会停止目标集群控制器（sdbcm）所在的机器所有节点

##参数##

* `businessName` ( *String*， *可选* )

	业务名。

##返回值##

成功：无。

失败：抛出异常。

##错误##

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][error_code]，或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。 可以参考[常见错误处理指南][faq]了解更多内容。

##版本##

v2.8 及以上版本

##示例##

1. 在本地停止所有业务名为 "yyy" 的节点

 	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.stopAllNodes( "yyy" )
    Stop sequoiadb(30000): Success
    Stop sequoiadb(30010): Success
    Stop sequoiadb(30020): Success
    Stop sequoiadb(20000): Success
    Stop sequoiadb(40000): Success
    Stop sequoiadb(41000): Success
    Stop sequoiadb(42000): Success
    Stop sequoiadb(50000): Success
    Total: 8; Success: 8; Failed: 0
 	```


[^_^]:
    本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
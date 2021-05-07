
##名称##

createCoord - 在目标集群控制器（sdbcm）所在的机器中创建一个 coord 节点。

##语法##

**oma.createCoord(\<svcname\>,\<dbpath\>,[config])**

##类别##

Oma

##描述##

在目标集群控制器（sdbcm）所在的机器中创建一个 coord 节点。一般情况，通过该接口创建的 coord 节点只出于临时使用的目的。

**Note:**

* oma 对象为连接到目标（本地/远端机器）集群控制器（sdbcm）获得的连接对象。

* 通过该接口创建的 coord 节点并不会注册到 catalog 中，即该 coord 节点不能被集群管理。若希望 coord 节点能够被集群管理，需要使用[createNode()](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/createNode.md)接口来创建 coord 节点。

##参数##

* `svcname` ( *Int | String*， *必填* )

	节点端口号。

* `dbpath` ( *String*， *必填* )

	节点数据目录。

* `config` ( *Object*， *选填* )

	节点配置信息，如配置日志大小，是否打开事务等，具体可参考[数据库配置](manual/Manual/Database_Configuration/configuration_parameters.md)。

##返回值##

成功：无。  

失败：抛出异常。

##错误##

`createCoord()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -3     | SDB_PERM | 权限错误。| 检查节点路径是否正确，路径权限是否正确。 |
| -15    | SDB_NETWORK | 网络错误。| 1. 检查 sdbcm 状态是否正常，如果状态异常，可以尝试重启。2. 检查网络情况。 |
| -145   | SDBCM_NODE_EXISTED | 节点已存在。| 检查节点是否存在。 |
| -157   | SDB_CM_CONFIG_CONFLICTS | 节点配置冲突。 | 检查端口号及数据目录是否已经被使用。 |

当异常抛出时，可以通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取[错误码](manual/Manual/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息。

可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)了解更多内容。

##版本##

v2.0及以上版本。

##示例##

在本地创建一个端口号为11810的 coord 节点，将该节点关联到指定的 catalog 节点。

```lang-javascript
> var oma = new Oma( "localhost", 11790 )
> oma.createCoord( 11810, "/opt/sequoiadb/database/coord/11810", { catalogaddr: "ubuntu1:11823, ubuntu2:11823" } )
```

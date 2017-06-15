##语法##
***rg.detachNode( \<host\>, \<service\>, [options] )***

分离当前分区组内的一个节点，其配置信息不会被删除。搭配 [rg.attachNode()](reference/Sequoiadb_command/SdbReplicaGroup/attachNode.md)使用。

##参数描述##

| 参数名  |  参数类型  |  描述                   |  是否必填 |
| ------- | ---------- | ----------------------- | --------- |
| host    |  string    | 节点的主机名或者主机 IP。  | 是 |
| service |  string    | 节点服务名或者端口。       | 是 |
| options |  Json 对象 | 可选项，详见如下options选项说明。 | 否 |

##options选项##

| 参数名  |  参数类型  |  描述                        |  默认值 |
| ------- | ---------- | ---------------------------- | ------- |
| KeepData  | bool     | 是否保留目标节点原有的数据。 |  false  |

> **Note:**
> 1. 主节点或分区组内只有一个节点时将不能被 detachNode 。
> 2. 分离后的节点将不再受集群管理，请尽快加入到其他复制组中。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrMsg](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息，或通过 [getLastError](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考 [常见错误处理指南](troubleshooting/general/general_guide.md) 。

##错误##

错误信息记录在节点诊断日志（diaglog）中，可参考[错误码](reference/Sequoiadb_error_code.md)。

| 错误码 | 可能的原因           | 解决方法 |
| ------ | -------------------- | ---------------------------------------------------- |
| -15    | 网络错误             | 1. 检查 sdbcm 状态是否正常；<br>2. 检查 host 是否正确，网络是否能正常通信。  |
| -155   | 节点不属于当前复制组 | 检查节点是否属于当前复制组。 |
| -204   | 主节点；或复制组内只存在一个节点 | 主节点或分区组内只有一个节点将不能被 detachNode ，请检查：<br>1. 是否为主节点；<br>2. 该分区组是否只有一个节点。 |

##示例##

见 [rg.attachNode()](reference/Sequoiadb_command/SdbReplicaGroup/attachNode.md) 中示例说明。


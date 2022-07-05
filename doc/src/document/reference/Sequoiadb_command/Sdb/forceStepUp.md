##名称##

forceStepUp - 强制将备节点升级为主节点

##语法##

**db.forceStepUp([options])**

##类别##

Sdb

##描述##

该函数用于在一个不具备选举条件的分区组中，将备节点强制升级为主节点。升级前需确保目标节点 LSN 为组内最大值。如果将 LSN 较小的节点强制升主，将导致数据回滚。用户可通过[节点健康检测快照](database_management/monitoring/snapshot/SDB_SNAP_HEALTH.md)获取节点 LSN 信息。

>**Note:**
>
> 该函数仅支持在编目分区组中使用。

##参数##

options（ *object，选填* ）

通过参数 options 可以指定主节点的持续时间：

- Seconds（ *number* ）：强制升级为主节点的持续时间，单位为秒，默认值为 120

    当超过指定时间，分区组内将按选举规则重新选主。

    格式：`Seconds: 300`

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取[错误码](reference/Sequoiadb_error_code.md)。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##

v3.2 及以上版本

##示例##

1. 连接编目节点 11800

    ```lang-javascript
    > var cata = new Sdb("localhost", 30000)
    ```

    >**Note:**
    >
    > 如果无法连接编目节点，需要将节点参数 auth 配置为 false，配置方式可参考[参数配置](database_management/database_configuration/configuration_parameters.md)。

2. 将编目节点 11800 强制升为主节点，并指定持续时间为 300 秒

    ```lang-javascript
    > cata.forceStepUp({Seconds: 300})
    ```
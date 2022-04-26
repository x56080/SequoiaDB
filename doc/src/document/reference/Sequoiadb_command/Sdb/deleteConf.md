##名称##

deleteConf - 删除指定的节点配置

##语法##

**db.deleteConf(\<config\>, [options])**

##类别##

Sdb

##描述##

该函数用于将指定的节点配置从配置文件中删除，已删除的配置将恢复为默认值。生效类型为“在线生效”的配置，删除后立即生效；生效类型为“重启生效”的配置，需重启节点后生效。各配置的生效类型可参考[参数说明](database_management/database_configuration/parameters_instructions.md)。

##参数##

| 参数名 | 类型 | 描述 | 是否必填 |
| ------ | ---- | ---- | -------- |
| config | object |[节点配置参数](database_management/database_configuration/parameters_instructions.md)，包含配置名和占位符<br>例如：{preferedinstance: 1, diaglevel: 1}，其中 1 没有特殊含义，仅作为占位符出现| 是 |
| options| object |[命令位置参数](reference/Sequoiadb_command/location.md)<br>如果不指定该参数，删除操作默认对所有节点生效| 否 |

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取[错误码](reference/Sequoiadb_error_code.md)。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##

v3.2 及以上版本

##示例##

- 删除“在线生效”类型的配置 diaglevel，并指定生效节点为 11820

    ```lang-javascript
    > db.deleteConf({diaglevel: 3}, {ServiceName: "11820"})
    ```

- 删除“重启生效”类型的配置 numpreload，并指定生效节点为 11820

    ```lang-javascript
    > db.deleteConf({numpreload: 1}, {ServiceName: "11820"})
    ```

    如果返回如下信息，需重启节点

    ```lang-javascript
    (shell):1 uncaught exception: -322
    Some configuration changes didn't take effect:
    Config 'numpreload' require(s) restart to take effect.
    ```
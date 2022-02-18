SequoiaDB 巨杉数据库支持通过动态配置和配置文件的方式进行参数配置。与配置文件方式相比，动态配置方式能够同时配置多个节点，用户可根据实际需求选择配置方式。配置完成后，可参考[配置快照](database_management/monitoring/snapshot/SDB_SNAP_CONFIGS.md)查看节点的配置信息。

##动态配置方式##

用户可通过 [updateConf()](reference/Sequoiadb_command/Sdb/updateConf.md) 和 [deleteConf()](reference/Sequoiadb_command/Sdb/deleteConf.md) 动态配置参数。如果参数的生效类型为“在线生效”，配置完成后立即生效；如果参数的生效类型为“重启生效”，配置完成后需重启节点才能使配置生效。生效类型可参考[参数说明](database_management/database_configuration/parameters_instructions.md)。

##配置文件方式##

用户可通过配置文件方式配置参数。如果参数的生效类型为“在线生效”，配置完成后需使用 [reloadConf()](reference/Sequoiadb_command/Sdb/reloadConf.md) 使配置生效；如果参数的生效类型为“重启生效”，配置完成后需重启节点才能使配置生效。以节点 11830 为例，具体操作如下：

1. 切换至数据库安装目录，以 `/opt/sequoiadb` 为例

    ```lang-bash
    $ cd /opt/sequoiadb
    ```

2. 修改配置文件 `conf/local/11830/sdb.conf`

    ```lang-bash
    $ vi conf/local/11830/sdb.conf
    ```

3. 写入需要修改的配置

4. 使配置生效

    “在线生效”类型的参数需执行如下命令：

    ```lang-javascript
    > var db = new Sdb("localhost", 11810)
    > db.reloadConf()

    ```

    “重启生效”类型的参数需执行如下命令：

    ```lang-bash
    $ sdbstop -p 11830
    $ sdbstart -p 11830
    ```


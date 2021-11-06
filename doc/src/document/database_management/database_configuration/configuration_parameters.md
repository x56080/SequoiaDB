SequoiaDB 巨杉数据库支持通过动态生效和配置文件的方式进行参数配置。如果需要查看指定节点的配置信息，可参考[配置快照](database_management/monitoring/snapshot/SDB_SNAP_CONFIGS.md)。

##动态生效方式##

用户可通过 [updateConf()](reference/Sequoiadb_command/Sdb/updateConf.md) 和 [deleteConf()](reference/Sequoiadb_command/Sdb/deleteConf.md) 动态配置参数。当参数的生效类型为“在线生效”时，动态配置后立即生效；当参数的生效类型为“重启生效”时，动态配置后需重启集群才能使配置生效。

##配置文件方式##

用户可通过配置文件方式配置参数。配置完成后，需使用 [reloadConf()](reference/Sequoiadb_command/Sdb/reloadConf.md) 使配置生效。以节点 11830 为例，具体操作如下：

1. 切换至数据库安装目录，以 `/opt/sequoiadb` 为例

    ```lang-bash
    $ cd /opt/sequoiadb
    ```

2. 修改配置文件 `conf/local/11830/sdb.conf`

    ```lang-bash
    $ vi conf/local/11830/sdb.conf
    ```

3. 在配置文件中新增如下内容：

    ```lang-ini
    diaglevel=5
    ```

4. 重新加载配置文件

    ```lang-javascript
    > db = new Sdb("localhost", 11810)
    > db.reloadConf()
    ```


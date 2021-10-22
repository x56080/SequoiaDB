SequoiaDB 巨杉数据库支持通过命令行、配置文件和动态生效的方式进行参数配置。用户可通过[配置快照][SDB_SNAP_CONFIGS]查看指定节点的配置信息。

##命令行方式##

当节点处于关闭状态时，用户可通过命令行方式配置参数。以节点 11830 为例，具体操作如下：

1. 切换至数据库安装目录，以 `/opt/sequoiadb` 为例

    ```lang-bash
    $ cd /opt/sequoiadb
    ```

2. 将参数 diaglevel 的值修改为 5

    ```lang-bash
    $ ./sequoiadb -c ../conf/local/11830/ --diaglevel 5 &
    ```

    > **Note:**
    >
    > 通过命令行方式修改的配置信息不会更新到配置文件中。如果需要更新配置文件，可参考 [flushConfigure()][flushConfigure] 。


##配置文件方式##

当节点处于启动状态时，用户可通过配置文件方式配置参数。配置完成后，需使用 [reloadConf()][reloadConf] 使配置生效。以节点 11830 为例，具体操作如下：

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

##动态生效方式##

用户可通过 [updateConf()][updateConf] 和 [deleteConf()][deleteConf] 动态配置参数。当参数的生效类型为“在线生效”时，动态配置后立即生效；当参数的生效类型为“重启生效”时，动态配置后需重启集群才能使配置生效。

[^_^]:
    本文使用的所有引用及链接
[updateConf]:Manual/Sequoiadb_Command/Sdb/updateConf.md
[deleteConf]:Manual/Sequoiadb_Command/Sdb/deleteConf.md
[reloadConf]:Manual/Sequoiadb_Command/Sdb/reloadConf.md
[SDB_SNAP_CONFIGS]:Manual/Snapshot/SDB_SNAP_CONFIGS.md
[flushConfigure]:Manual/Sequoiadb_Command/Sdb/flushConfigure.md
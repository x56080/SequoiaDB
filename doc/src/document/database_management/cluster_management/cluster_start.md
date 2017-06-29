##操作系统重启动##

操作系统启动后会自动启动服务 sdbcm（SequoiaDB Cluster Manager）。该服务启动后会自动启动该物理机中所有注册在 /opt/sequoiadb/conf/local 目录下的节点。使用命令 ```ps –elf | grep sequoiadb``` 能看到当前正在启动的节点与启动完毕的节点。启动完毕的进程名为：sequoiadb（服务名）, 正在启动的进程名一般为：

```lang-javascript
$ /opt/sequoiadb/bin/sequoiadb –c /opt/sequoiadb/conf/local/（服务名）
```

##手工启动特定节点##

当集群中某个节点失效后，用户可以在 sdb 命令行使用如下步骤启动节点。（假设 SequoiaDB 的安装路径为 ```/opt/sequoiadb```）

1.  连接到协调节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var db = new Sdb( "localhost", 11810 )
    ```

2.  得到分区组

    ```lang-javascript
    > dataRG = db.getRG( "<datagroup1>" )
    ```

3.  得到数据节点

    ```lang-javascript
    > dataNode = dataRG.getNode( "<hostname1>", "<servicename1>" )
    ```

4.  启动节点

    ```lang-javascript
    > dataNode.start()
    ```

##手工启动数据组##

当集群中某个数据组被停止后，用户可以在 sdb 命令行使用如下步骤启动数据组。该操作会启动数据组中全部数据节点。

1.  连接到协调节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var db = new Sdb( "localhost", 11810 )
    ```

2.  得到分区组

    ```lang-javascript
    > dataRG = db.getRG( "<datagroup1>" )
    ```

3.  启动数据组

    ```lang-javascript
    > dataRG.start()
    ```

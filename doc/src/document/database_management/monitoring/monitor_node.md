用户可以使用 snapshot 监控每个节点的状态。

1.  连接到协调节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var db = new Sdb( "localhost", 11810 )
    ```

2.  得到分区组

    ```lang-javascript
    > datarg = db.getRG( "< datagroup1 >" )
    ```

3.  得到数据节点

    ```lang-javascript
    > datanode = datarg.getNode( "< hostname1 >", "< servicename1 >" )
    ```

4.  得到该节点的快照

    ```lang-javascript
    > datanode.connect().snapshot( SDB_SNAP_DATABASE )
    ```

>   **Note:**
>
>   [快照](database_management/monitoring/snapshot/snapshot.md) 类型分为：
>
>   *   [上下文快照](database_management/monitoring/snapshot/SDB_SNAP_CONTEXTS.md)
>   *   [当前会话上下文快照](database_management/monitoring/snapshot/SDB_SNAP_CONTEXTS_CURRENT.md)
>   *   [会话快照](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS.md)
>   *   [当前会话快照](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS_CURRENT.md)
>   *   [集合快照](database_management/monitoring/snapshot/SDB_SNAP_COLLECTIONS.md)
>   *   [集合空间快照](database_management/monitoring/snapshot/SDB_SNAP_COLLECTIONSPACES.md)
>   *   [数据库快照](database_management/monitoring/snapshot/SDB_SNAP_DATABASE.md)
>   *   [系统快照](database_management/monitoring/snapshot/SDB_SNAP_SYSTEM.md)
>   *   [编目信息快照](database_management/monitoring/snapshot/SDB_SNAP_CATALOG.md)

用户可以使用 Shell 脚本监控，例如“monitor_insert.sh”：

```
#!/bin/bash
~/sequoiadb/bin/sdb "db=new Sdb('hostname1',11810); \
                     db.getRG('foo').getNode('hostname2',11820).connect().snapshot(SDB_SNAP_DATABASE)" \
                     | grep TotalInsert
```

运行结果：

```
$ ./monitor_insert.sh
"TotalInsert": 0,
```

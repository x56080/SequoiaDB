##手工停止特定节点##

用户可以在 sdb 命令行使用如下步骤停止数据节点。

1.  连接到协调节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var db = new Sdb( "localhost", 11810 )
    ```

2.  得到分区组

    ```lang-javascript
    > dataRG = db.getRG( "<datagroup1>" )
    ```

3.   得到数据节点

    ```lang-javascript
    > dataNode = dataRG.getNode( "<hostname1>", "<servicename1>" )
    ```

4.  停止节点

    ```lang-javascript
    > dataNode.stop()
    ```

##手工停止数据组##

用户可以在 sdb 命令行使用如下步骤停止数据组。该操作会停止数据组中全部数据节点。

1.  连接到协调节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var db = new Sdb( "localhost", 11810 )
    ```

2.  得到分区组

    ```lang-javascript
    > dataRG = db.getRG( "<datagroup1>" )
    ```

3.  停止数据组

    ```lang-javascript
    > dataRG.stop()
    ```

##使用kill命令停止数据节点##

用户可以使用 ```kill -15 <pid>``` 正常停止数据节点。以该方式停止的数据节点被认为正常停止。用户使用 ```kill -9 <pid>``` 强行停止数据节点。以该方式停止的数据节点被认为非正常停止。如果该节点非正常停止，则会被 sdbcm 进程尝试重新启动。启动后会与当前数据组中其它节点进行同步。

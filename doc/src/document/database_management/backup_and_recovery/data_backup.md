当前版本中，数据库备份支持全量备份和增量备份。全量备份过程中会阻塞数据库变更操作，即数据插入、更新、删除等变更操作会被阻塞直到全量备份完成才会执行；增量备份过程中不阻塞数据库变更操作。

*   全量备份：备份整个数据库的配置、数据和日志（可选）；
*   增量备份：在上一个全量备份或增量备份的基础上备份新增的日志和配置；增量备份需要保证日志的连续性和一致性，如果日志不连续，或日志Hash校验不一致，则增量备份失败。因此，周期性的增量备份需要计算好日志和周期的关系，以防止日志覆写。

##离线备份参数说明##

使用 [Sdb.backupOffline()](reference/Sequoiadb_command/Sdb/backupOffline.md) 命令可以进行离线备份，以下是常用参数说明：

| 参数        | 说明 |
| ----------- | ---- |
| Name        | 备份名称，缺省则以当前时间格式命名，如“2016-01-01-15:00:00”，格式为“YYYY-MM-DD-HH:mm:ss”。 |
| Description | 备份用户描述信息。 |
| Path        | 本次备份的指定路径，缺省为配置参数“bkuppath”中指定的路径。 |
| EnsureInc   | 备份方式，true 表示增量备份，false 表示全量备份，缺省为 false。 |
| OverWrite   | 对于同名备份是否覆盖，true 表示覆盖，false 表示不覆盖，如果同名则报错；缺省为 false。 |
| GroupName   | 对指定组进行备份，缺省为对全系统备份，当需要对多个组进行备份可以指定为数组类型，如：```["datagroup1", "datagroup2"]```。 |

##全量备份整个数据库##

1.  连接到协调节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var db = new Sdb( "localhost", 11810 )
    ```

2.  执行全量备份命令

    ```lang-javascript
    > db.backupOffline( { Name: "backupName", Description: "backup for all" } )
    ```

##全量备份指定组的数据库##

1.  连接到协调节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var db = new Sdb( "localhost", 11810 )
    ```

2.  执行全量备份命令

    ```lang-javascript
    > db.backupOffline( { Name: "backupName", Description: "backup group1", GroupName: "group1" } )
    ```

##全量+增量备份指定节点的数据库##

1.  连接到指定节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var dbdata = new Sdb( "hostname1", "servicename1" )
    ```

2.  执行全量备份命令

    ```lang-javascript
    > dbdata.backupOffline( { Name: "backupName", Description: "backup data node" } )
    ```

3.  后续可以定期执行增量备份命令

    ```lang-javascript
    > dbdata.backupOffline( { Name: "backupName", Description: "increase backup data node", EnsureInc: true } )
    ```

>   **Note:**
>  
>   在协调节点上对整个数据库或指定组进行备份，默认是只在该数据组主节点上进行备份。  
>   Catalog 编目组的名称固定为 SYSCatalogGroup

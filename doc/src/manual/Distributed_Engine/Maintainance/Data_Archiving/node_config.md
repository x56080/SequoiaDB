
## 配置方式

节点配置主要以配置文件方式，用户可通过配置文件方式配置参数，配置完成后需重启节点才能使配置生效。以修改调度服务节点 9000 为例，具体操作如下：

1. 切换至 sdb-schedule 安装目录，以 /opt/data/sdb-schedule/ 为例
    
    ```shell
    $ cd /opt/data/sdb-schedule/
    ```

2. 编辑配置文件 conf/schedule-server/9000/application.properties

    ```shell
    $ vi conf/schedule-server/9000/application.properties
    ```
   
3. 写入需要修改的配置，重启节点使配置生效

## 详细配置说明

| 配置项                                                        | 类型   | 说明                                                                 | 生效类型 |
|------------------------------------------------------------|------|--------------------------------------------------------------------|------|
| system.store.sequoiadb.urls                                | str  | 元数据服务 SequoiaDB 的协调节点服务地址。例如：192.168.20.56:11810,192.168.20.57:11810 | 重启生效 |
| system.store.sequoiadb.username                            | str  | 登录 SequoiaDB 的用户名。例如：sdbadmin，默认用户名为空                              | 重启生效 |
| system.store.sequoiadb.password                            | str  | 登录 SequoiaDB 的密码 md5 加密串                                           | 重启生效 |
| system.store.sequoiadb.connectTimeout                      | num  | 连接超时时间，默认值：10000，单位：毫秒                                             | 重启生效 |
| system.store.sequoiadb.maxAutoConnectRetryTime             | num  | 最大重连间隔，默认值：15000，单位：毫秒                                             | 重启生效 |
| system.store.sequoiadb.socketTimeout                       | num  | socket 超时时间，默认值：0（不检测超时），单位：毫秒                                     | 重启生效 |
| system.store.sequoiadb.useNagle                            | bool | 是否开启 Nagle，默认值：false                                               | 重启生效 |
| system.store.sequoiadb.useSSL                              | bool | 是否使用 SSL 安全连接，默认值：false                                            | 重启生效 |
| system.store.sequoiadb.maxConnectionNum                    | num  | SequoiaDB 连接池最大连接数，默认值：500                                         | 重启生效 |
| system.store.sequoiadb.deltaIncCount                       | num  | 当需要新增连接时，一次新增的连接数，默认值：10                                           | 重启生效 |
| system.store.sequoiadb.maxIdleNum                          | num  | 最大空闲连接数，默认值：2                                                      | 重启生效 |
| system.store.sequoiadb.keepAliveTime                       | num  | 连接池保留空闲连接的时长，默认值：60000（不清除空闲连接），单位：毫秒                              | 重启生效 |
| system.store.sequoiadb.recheckCyclePeriod                  | num  | 清理空闲连接的间隔时间。默认值：30000，单位：毫秒                                        | 重启生效 |
| system.store.sequoiadb.validateConnection                  | bool | 出池时是否检查连接有效性, 默认值：true                                             | 重启生效 |
| system.jvm.options                                         | str  | 配置 java jvm 参数，例如：-Xmx2048M -Xms2048M -Xmn1536M                    | 重启生效 |
| logging.level.com.sequoiadb.datasource.SequoiadbDatasource | str  | 设置 SequoiaDB 连接池日志级别，默认值：WARN | 重启生效 |
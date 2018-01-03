##描述##

  数据库服务sdbcm是一个守护进程。SequoiaDB 的所有集群管理操作都必须有 sdbcm 的参与，且每一台物理机器上只能启动一个 sdbcm 进程，负责执行远程的集群管理命令和监控本地的 SequoiaDB 数据库。

##数据库服务相关参数说明##


1.  启动服务

    ```lang-javascript
    $ service sdbcm start
    ```

2.  停止服务

    ```lang-javascript
    $ service sdbcm stop
    ```

3.  查看服务状态，系统提示“sdbcm is running”表示服务正在运行

    ```lang-javascript
    $ service sdbcm status
    ```

4.  重启服务

    ```lang-javascript
    $ service sdbcm restart
    ```

    >**Note:**  
    > 在/proc/1/exe所指向的启动文件为/lib/systemd/systemd的系统上，执行service sdbcm restart重启cm和节点。 

    ```lang-javascript
    $ service sdbcm restart all-nodes
    ``` 

    >**Note:**  
    > 在/proc/1/exe所指向的启动文件为/sbin/init的系统上，执行service sdbcm restart all-nodes重启cm和节点。
    
5.  强制重新加载

    ```lang-javascript
    $ service sdbcm force-reload
    ```

    >**Note:**  
    > 在/proc/1/exe所指向的启动文件为/lib/systemd/systemd的系统上，执行servcie sdbcm force-reload重启cm进程和节点进程。

    ```lang-javascript
    $ service sdbcm force-reload all-nodes
    ```

    >**Note:**  
    > 在/proc/1/exe所指向的启动文件为/sbin/init的系统上，执行service sdbcm force-reload all-nodes重启cm进程和节点进程。

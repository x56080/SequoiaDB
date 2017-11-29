sdbsupport 是用于收集 SequoiaDB 相关信息的工具，位于目录 /opt/sequoiadb/tools 下面。此工具收集的信息包括：数据库配置信息、数据库日志信息、数据库所在主机的硬件信息和数据库、操作系统信息以及数据库快照信息。

使用此工具需要先为 sdbsupport.sh 赋执行权限：

```lang-javascript
$ chmod 755 sdbsupport.sh
```

##权限需求##

数据库用户权限。

##选项##

| 参数              | 缩写 | 描述                                     |
| ----------------- | ---- | ---------------------------------------- |
| --help            |      | 帮助选项                                 |
| --hostname        | -s   | 所需要收集的信息的主机名字               |
| --svcname         | -p   | 指定特定端口收集其配置、日志及快照信息   |
| --user            | -u   | 数据库用户名                             |
| --password        | -w   | 数据库用户密码                           |
| --snapshot        | -n   | 收集快照信息                             |
| --osinfo          | -o   | 收集操作系统信息                         |
| --hardware        | -h   | 收集硬件信息                             |
| --all             |      | 指定收集数据库所有信息                   |
| --conf            |      | 指定收集配置文件的信息                   |
| --log             |      | 指定收集日志文件信息                     |
| --cm              |      | 指定收集 CM 配置、日志信息               |
| --cpu             |      | 指定收集 CPU 信息                        |
| --memory          |      | 指定收集内存信息                         |
| --disk            |      | 指定收集硬盘信息                         |
| --netcard         |      | 指定收集网卡信息                         |
| --mainboard       |      | 指定收集主板信息                         |
| --catalog         |      | 指定收集编目节点快照                     |
| --group           |      | 指定收集数据库集群组的信息               |
| --context         |      | 指定收集上下文快照信息                   |
| --session         |      | 指定收集会话快照信息                     |
| --collection      |      | 指定收集集合快照信息                     |
| --collectionspace |      | 指定收集集合空间信息                     |
| --database        |      | 指定收集数据库快照信息                   |
| --system          |      | 指定收集系统快照信息                     |
| --diskmanage      |      | 指定收集操作系统硬盘管理信息             |
| --basicsys        |      | 指定收集操作系统基本信息                 |
| --module          |      | 指定收集内核加载模块信息                 |
| --env             |      | 指定收集操作系统环境变量信息             |
| --network         |      | 指定收集 IP 地址等网络信息               |
| --process         |      | 指定收集操作系统进程信息                 |
| --login           |      | 指定收集用户登陆此机所进行操作的历史信息 |
| --limit           |      | 指定收集操作系统用户限制信息             |
| --vmstat          |      | 指定收集给定时间间隔内的服务状态值信息   |

##用法示例##

1.  获取参数信息。

    ```lang-javascript
    $ ./sdbsupport.sh --help
    ```

2.  收集本机的数据信息。（包括配置、日志、硬件、操作系统及快照信息）

    ```lang-javascript
    $ ./sdbsupport.sh
    ```

3.  收集整个数据库集群信息

    ```lang-javascript
    $ ./sdbsupport.sh --all
    ```

4.  收集指定主机信息。

    ```lang-javascript
    $ ./sdbsupport.sh -s hostname1
    ```

5.  收集指定主机指定端口信息。

    ```lang-javascript
    $ ./sdbsupport.sh -s hostname1 -p 11810
    ```

6.  收集操作系统信息。

    ```lang-javascript
    $ ./sdbsupport.sh --osinfo
    ```

7.  收集特定主机特定端口的日志信息及快照信息。

    ```lang-javascript
    $ ./sdbsupport.sh -s hostname1 -p 11810 --snapshot --log
    ```

##信息归类##

通过执行“sdbsupport.sh”收集的数据库信息信息，会全部收集到本地的“log”文件夹中。收集的信息是以主机为单位打成的压缩包，名称以“主机名-年月日-时分秒”命名。将此文件解压缩后会得四个文件夹：SDBNODES，SDBSNAPS，OSINFO，HARDINFO。

*   SDBNODES：存放收集的数据库配置、日志信息
*   SDBSNAPS：存放收集的数据库快照信息
*   OSINFO：存放收集的操作系统信息
*   HARDINFO：存放收集的硬件信息

>   **Note:**
>
>   数据库集群内的机器，如果没有配置信任关系，在收集时，需要输入密码，如：
>
>   ```lang-javascript
>   $ /opt/sequoiadb/tools/sdbsupport/sdbsupport.sh -s hostname2
>   ************************************Sdbsupport***************************
>   *This program run mode will collect all configuration and
>   * system environment information.Please make sure whether
>   * you need !
>   * Begin .....
>   *************************************************************************
>   
>   check over environment, correct!
>   complete database cluster
>   The host sdbadmin@hostname2's password :
>   ```
>
>   此时需要输入 hostname2 机器，sdbadmin 用户的密码，然后“Enter”
>
>   ```lang-javascript
>   correct password for hostname2
>   Begin to Collect information...
>   success to collect information from hostname2
>   ```

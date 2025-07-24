stplist 是 SequoiaDB 巨杉数据库的 STP 节点检测工具，用于查看本地机器中 STP 节点及节点的状态信息。

SequoiaDB STP 节点启动后，会将自身的状态信息写入管道文件，供 stplist 工具获取。STP 节点的管道文件存放于 `/var/sequoiadb` 目录。当管道文件不存在时，stplist 工具将无法显示节点的部分信息。

## 参数说明

| 参数名    | 缩写 | 描述|
| ----      | ---- | ----|
| --help    | -h   | 显示帮助信息 |
| --mode    | -m   | 指定需要操作的节点类型，默认为"run"，取值如下：<br>"run"：正在运行的节点<br> "local"：本地的节点|
| --long    | -l   | 显示详细信息 |
| --version |      | 显示版本号 |
| --detail  |      | 显示节点配置文件信息 |
| --expand  |      | 显示节点所有项信息 |

## 示例

- 显示 STP 节点

    ```lang-bash
    $ bin/stplist
    stp(9622) (19981)
    Total: 1
    ```

- 显示 STP 节点详细信息

    ```lang-bash
    $ bin/stplist -l
    Name       SvcName       PID       PRY  StartTime
    stp        9622          19981     Y    2021-03-11-14.23.09
    Total: 1
    ```

- 显示 STP 节点配置文件信息

    ```lang-bash
    $ bin/stplist --detail
    stp(9622) (19981)
       serverlist        : sdbserver1:9622
       role              : server
    Total: 1
    ```

- 显示 STP 节点所有配置信息

    ```lang-bash
    $ bin/stplist --expand
    stp(9622) (19981)
       port              : 9622
       serverlist        : sdbserver1:9622
       role              : server
       weight            : 0
       syncinterval      : 60
       maxtimeerror      : 50000
       diaglevel         : 3
    Total: 1
    ```

- 显示停止运行的 STP 节点及配置

    ```lang-bash
    $ bin/stplist --mode local --detail
    stp(9622) (-)
       serverlist        : sdbserver1:9622
       role              : server
    Total: 1
    ```

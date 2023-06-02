[^_^]:
    适配器管理工具

sdbseactl 是 SequoiaDB 巨杉数据库的适配器管理工具，用于管理[全文索引][text_index]的搜索引擎适配器节点。适配器节点启动后，会将自身的状态信息写入管道文件，供 sdbseactl 工具获取。管道文件存放于 `/var/sequoiadb` 目录，当管道文件不存在时，工具将无法显示节点的部分信息。

##语法规则##

**sdbseactl <--mode|-m arg> [--svcname|-p arg] [--all|-a] [--long|-l] [--force]**

##参数说明##

| 参数名         | 缩写 | 描述                                     |
| -------------- | ---- | ---------------------------------------- |
| --help         | -h   | 获取帮助信息     |
| --version      | -v   | 获取版本信息     |
| --mode         | -m   | 指定 sdbseactl 的工作模式，取值列表如下：<br>● start：启动适配器节点<br>● stop：中止适配器节点<br>● list：查看本地机器的适配器节点信息 |
| --svcname      | -p   | 指定需要操作的适配器节点端口号，多个端口号间采用逗号（,）分隔 |
| --all          | -a   | 对所有适配器节点进行操作  |
| --long         | -l   | 在 list 模式下，展示节点的详细信息       |
| --force        | -    | 强制停止节点       |
| --ignoreulimit | -i   | 启动适配器节点时，忽略 ulimit 检查       |

##常见场景##

###启动适配器节点###

- 启动所有已配置的适配器节点

    ```lang-bash
    $ sdbseactl -m start -a
    ```

- 启动指定的适配器节点

    ```lang-bash
    $ sdbseactl -m start -p 11827,11837
    ```

###查看适配器节点信息###

- 查看所有正在运行的适配器节点

    ```lang-bash
    $ sdbseactl -m list
    ```

    输出结果如下：

    ```lang-text
    sdbseadapter(11847) (29010) A
    sdbseadapter(11837) (29013) A
    ```

- 查看适配器节点的详细信息

    ```lang-bash
    $ sdbseactl -m list -l
    ```

    输出结果如下：

    ```lang-text
    Name          SvcName      Role        PID       DataSvcName   Mode        StartTime
    sdbseadapter  11847        seadapter   29010     11840         read-write  2023-04-27-10.30.44 
    sdbseadapter  11837        seadapter   29013     11830         read-write  2023-04-27-10.30.44 
    ```

- 查看指定的适配器节点

    ```lang-bash
    $ sdbseactl -m list -p 11847
    ```

    输出结果如下：

    ```lang-text
    sdbseadapter(11847) (29010) A
    ```

###停止适配器节点###

- 停止所有适配器节点

    ```lang-bash
    $ sdbseactl -m stop -a
    ```

- 停止指定的适配器节点

    ```lang-bash
    $ sdbseactl -m stop -p 11827,11837
    ```

[^_^]:
     本文使用的所有引用及链接
[text_index]:manual/Distributed_Engine/Operation/Index/text_index.md
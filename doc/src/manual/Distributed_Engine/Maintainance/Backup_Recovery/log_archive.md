[^_^]:
    日志归档与重放

SequoiaDB 巨杉数据库支持日志归档与重放功能。在数据库引擎运行的过程中，同步日志将被循环使用，新的日志会覆盖旧的日志。因此当用户希望系统可以自动执行备份操作时，应开启日志归档功能。已归档的日志可以通过重放功能在其他集群或节点重新执行。通过日志归档与重放，用户可以实现不同集群间的数据同步。

##日志归档##

开启日志归档功能后，SequoiaDB 将根据触发归档的条件，在目录 `archivelog` 下自动生成归档文件，并记录同步日志的内容。

触发归档的条件如下：

- 当前同步日志文件写满并切换至下一同步日志时
- 归档超时时间段内未发生归档操作

###归档文件###

归档文件分为完整归档文件、部分归档文件和发生移动的归档文件，具体说明如下：

- 完整归档文件：归档已写满的同步日志文件。当前同步日志文件写满并切换至下一个文件时，将生成该类型的文件。
- 部分归档文件：归档未写满的同步日志文件。如果参数 [archivetimeout][node_conf] 指定的时间内未触发归档，将根据当前使用的同步日志生成该类型的文件。
- 发生移动的归档文件：当节点由于某些原因（节点主降备、异常重启等）导致同步日志的 LSN 发生改变，已生成的归档文件都将转换为该类型的文件。

归档文件对应的格式如下：

| 类型               | 文件格式                |
| ------------------ | ----------------------- |
| 完整归档文件       | archivelog.\<FileId\>   |
| 部分归档文件       | archivelog.\<FileId\>.p |
| 发生移动的归档文件 | archivelog.\<FileId\>.m |

>**Note:**
>
> - FileId 是顺序增长的序列号。
> - 当前日志写满后，部分归档文件将自动转换为完整归档文件。
> - 如果归档日志开启了压缩功能，节点将会对完整归档文件进行压缩。
> - 同步日志归档功能详细配置，可参考节点[配置文件][node_conf]中 archiveon、archivecompresson、archivepath、archivetimeout、archiveexpired 和 archivequota 参数介绍。


###开启归档###

首次开启归档功能后，SequoiaDB 将在目录 `archivelog` 下生成状态文件 `.archive.1` 和 `.archive.2`，用于记录归档的起始 LSN。下述以节点 11820 和 11830 为例，介绍开启归档功能的具体步骤。

1. 将参数 archiveon 设置为 true

    ```lang-javascript
    > db.updateConf({archiveon: true}, {svcname: ["11820","11830"]})
    (shell):1 uncaught exception: -322
    Some configuration changes didn't take effect:
    Config 'archiveon' require(s) restart to take effect.
    ```

2. 重启节点使配置生效

    ```lang-bash
    $ sdbstop -p 11820,11830
    $ sdbstart -p 11820,11830
    ```

3. 查看归档功能的配置信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CONFIGS, {SvcName: ["11820", "11830"]}, {svcname:"", archiveon: "", archivecompresson: "", archivepath: "", archivetimeout: "", archiveexpired: "", archivequota: ""})
    ```

    输出结果如下，字段 archiveon 显示为 "TRUE" 表示成功开启归档：

    ```lang-javascript
    {
      "svcname": "11820",
      "archiveon": "TRUE",
      "archivecompresson": "TRUE",
      "archivepath": "/opt/sequoiadb/database/data/11820/archivelog/",
      "archivetimeout": 600,
      "archiveexpired": 240,
      "archivequota": 10
    }
    {
      "svcname": "11830",
      "archiveon": "TRUE",
      ...
    }
    ```

>**Note:**
>
> 归档功能的相关配置可参考[参数说明][node_conf]。

##日志重放##

SequoiaDB 支持使用 [sdbreplay 工具][repli_tool]重放归档文件或归档目录。下述以主机 `sdbserver:11810`、集合 sample.employee、归档文件 `archivelog.0.p` 为例，介绍日志重放的具体步骤。

1. 检查集群是否存在集合 sample.employee

    ```lang-javascript
    > db.listCollections()
    ```

    如果集合不存在，用户需手动创建

    ```lang-javascript
    > db.createCS("sample").createCL("employee")
    ```

2. 重放归档文件 `archivelog.0.p`

    ```lang-bash
    $ sdbreplay --hostname sdbserver --svcname 11810 --path /data/archivelog/archivelog.0.p
    ```

3. 检查集合 sample.employee 中是否新增对应条数的记录

    ```lang-javascript
    > db.sample.employee.find().count()
    ```

>**Note:**
>
> - 指定重放归档目录时，将跳过 .m 文件，并根据归档文件的 Filed 从小到大依次重放。
> - 当发生数据回滚时，用户可单独重放 .m 文件找回丢失的数据。

[^_^]:
    本文使用到的所有链接及引用。
[node_conf]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
[repli_tool]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/log_replay.md
[sdb_diaglog]:manual/Distributed_Engine/Maintainance/DiagLog/diaglog.md
[sycn_log]:manual/Distributed_Engine/Architecture/Replication/architecture.md##同步日志
[repli]:manual/Distributed_Engine/Maintainance/Backup_Recovery/log_archive.md##日志重放
[archive_file]:manual/Distributed_Engine/Maintainance/Backup_Recovery/log_archive.md##归档文件
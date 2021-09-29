SequoiaDB支持命令行方式及配置文件方式进行参数配置。

###命令行方式配置###

在启动sequoiadb时传入配置参数值：

```lang-bash
$ ./sequoiadb --businessname yyy --catalogaddr ubuntu-wjm:30003,ubuntu-wjm:30013,ubuntu-wjm:30023 --clustername xxx --dbpath /home/users/wjm/sequoiadb/trunk/50000 --diaglevel 3 --role coord --svcname 50000
```

###配置文件方式配置###

在启动sequoiadb时传入配置文件路径：

```lang-bash
$ ./sequoiadb -c ../conf/local/50000/
```

配置文件内容如下：

```lang-ini
businessname=yyy
catalogaddr=ubuntu-wjm:30003,ubuntu-wjm:30013,ubuntu-wjm:30023
clustername=xxx
dbpath=/home/users/wjm/sequoiadb/trunk/50000
diaglevel=3
role=coord
svcname=50000
```

>**Note:**  
>1. 当两种方式并存时，命令行参数将会覆盖配置文件中的相同的配置项。 

###配置动态生效###
使用 [updateConf()][updateConf()] 以及 [deleteConf()][deleteConf()] 在线修改配置。

使用 [reloadConf()][reloadConf()] 重新加载配置文件，并进行配置动态生效，只支持“生效类型”列为“在线生效”的配置项，其他配置项会被忽略。“生效策略”若无其他说明，则默认为立即生效。

[^_^]:
     本文使用的所有引用及链接
[updateConf()]:manual/Manual/Sequoiadb_Command/Sdb/updateConf.md
[deleteConf()]:manual/Manual/Sequoiadb_Command/Sdb/deleteConf.md
[reloadConf()]:manual/Manual/Sequoiadb_Command/Sdb/reloadConf.md
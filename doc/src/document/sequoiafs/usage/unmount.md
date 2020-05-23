本章介绍如何卸载已挂载的 SequoiaFS 目录。

##fsstop.sh 卸载##

卸载脚本位于 SequoiaFS 安装目录的bin目录中。

指定挂载目录名称卸载。

```lang-bash
$ ./fsstop.sh -m /opt/sequoiadb/guestdir
```

或使用别名卸载指定目录（--alias 参数指定挂载目录的别名）。

```lang-bash
$ ./fsstop.sh --alias guestdir
```
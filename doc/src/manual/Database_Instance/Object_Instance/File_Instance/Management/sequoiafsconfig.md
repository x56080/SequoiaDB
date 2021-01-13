本文档主要介绍 SequoiaFS 的配置参数及配置文件路径创建规则。

##配置文件路径说明##

查询 SequoiaDB 安装信息

```lang-bash
$ . /etc/default/sequoiadb
$ echo $INSTALL_DIR
```

输出结果如下：

```lang-text
/opt/sequoiadb
```

配置文件路径：`$INSTALL_DIR/tools/sequoiafs/conf/local/$alias/`  
配置文件名称：`sequoiafs.conf`

- $INSTALL_DIR 为 SequoiaDB 安装路径；
- `$INSTALL_DIR/tools/sequoiafs` 为 SequoiaFS 所在路径；
- $alias 为挂载目录别名，别名默认为挂载目录全路径的最后一级目录名称，不同挂载目录应有不同的别名。以挂载目录为 `/home/sdbadmin/guestdir/`，别名为"guestdir"为例，该挂载目录对应的配置文件路径应为 `/opt/sequoiadb/tools/sequoiafs/conf/local/guestdir/`。

在 `$INSTALL_DIR/tools/sequoiafs/conf/sample/` 目录中有配置样例 `sequoiafs.conf`，可以将样例复制到对应配置文件路径，并在此基础上进行修改。

##参数说明##

下列参数除 help 和 version 外，都可以在配置文件中使用，也可以作为启动参数使用。

| 参数名 | 缩写 | 描述 |
| ----   | ---- | ---- |
| --help | -h   | 显示帮助信息 |
| --version | -v | 显示版本信息 |
| --mountpoint | -m | 挂载目录的全路径 |
| --alias | | 挂载目录的别名，通常为挂载目录全路径的最后一级目录  |
| --hosts | -i | 指定需要映射的集合的所属主机节点地址（hostname:svcname），用","分隔多个地址，默认值为 `localhost:11810` |
| --username | -u | 数据库用户名 |
| --passwd | -p | 数据库密码 |
| --collection | -l | 指定需要映射的目标集合全称，包括集合空间名称和集合名称，例如:"mountcs.mountcl" |
| --metadircollection | -d | 指定目录元数据集合全称，如不指定，则默认根据目标集合生成对应集合名称，当目标集合全称为"mountcs.mountcl"，默认生成的目录元数据集合全称为"mountcs.mountcl_FS_SYS_DirMeta" |
| --metafilecollection | -f   | 指定文件元数据集合全称，如不指定，则默认根据目标集合生成对应集合名称，当目标集合全称为"mountcs.mountcl"，默认生成的文件元数据集合全称为"mountcs.mountcl_FS_SYS_FileMeta" |
| --connectionnum | -n | 指定连接池最大支持连接数大小，取值范围为 50~1000，默认值为 100 |
| --cachesize | -s | 目录 LRU 缓存大小，单位为 M，取值范围为 1~200，默认值为 2 |
| --confpath | -c | 配置文件路径 |
| --diaglevel | -g | 设置日志级别，取值范围为 0~5，默认值为 3 |
| --replsize | -r | 指定元数据集合创建时的 ReplSize，取值范围为 -1~7，默认值为 2 |
| --diagnum | | 指定日志文件最大个数，-1 表示无限制，默认值为 20 |
| --diagpath | | 指定日志文件目录 |
| --fuse_allow_other | |  FUSE 参数，是否允许挂载用户以外的其他用户访问挂载目录，默认值为 true | 
| --fuse_big_writes |  |FUSE 参数，是否允许超过 4KB 的写操作，默认值为 true，最大为 32KB |
| --fuse_max_write | | FUSE 参数，指定 write 请求的最大 size，默认值为 131072 |
| --fuse_max_read | | FUSE 参数，指定 read 请求的最大 size，默认值为 131072 | 


>**Note:**
>   
>* SequoiaFS 可以配置 FUSE 参数，但是需要在 FUSE 参数前加“fuse_”作为前缀以便和 SequoiaFS 的参数进行区分。例如：-o nonempty，需转换为 --fuse_nonempty true，-o max_read=N，需转换为 --fuse_max_read N。

##参数配置##

SequoiaFS 支持命令行及配置文件进行参数配置，所有参数均需重启生效。

> **Note:**
>
> 启动脚本会根据配置文件路径生成规则获得配置文件路径，以命令行和配置文件结合的方式启动，命令行指定参数会覆盖配置文件中相同的配置，挂载结束后命令行参数会写入配置文件。

- **配置文件方式**
  
   启动时只指定配置文件路径

   ```lang-bash
   $ ./fsstart.sh -c /opt/sequoiadb/tools/sequoiafs/conf/local/guestdir
   ```

   配置文件内容如下：

   ```lang-text
   mountpoint=/home/sdbadmin/guestdir
   alias=guestdir
   collection=mountcs.mountcl
   metafilecollection=mountcs.mountcl_FS_SYS_FileMeta
   metadircollection=mountcs.mountcl_FS_SYS_DirMeta
   confpath=/opt/sequoiadb/tools/sequoiafs/conf/local/guestdir/
   diagpath=/opt/sequoiadb/tools/sequoiafs/log/guestdir/
   fuse_allow_other=true
   ```
   
   >**Note：**
   >
   > --help 与 --version 参数不支持在配置文件中进行配置。

- **命令行方式**

   启动时指定挂载目录、别名和挂载集合

   ```lang-bash
   $ ./fsstart.sh -m /home/sdbadmin/guestdir --alias guestdir --collection mountcs.mountcl
   ```



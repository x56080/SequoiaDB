本章介绍 SequoiaFS 的配置及配置文件使用规则。

##配置文件及日志路径##

查询 SequoiaDB 安装信息。

```lang-bash
$ . /etc/default/sequoiadb
$ echo $INSTALL_DIR
```
```lang-text
/opt/sequoiadb
```

INSTALL_DIR 为 SequoiaDB 的安装目录，SequoiaFS 目录为 $INSTALL_DIR/tools/sequoiafs。

配置文件路径及日志文件路径创建规则：

- SequoiaFS 配置文件路径为 $INSTALL_DIR/tools/sequoiafs/conf/local 

- SequoiaFS 日志文件路径为 $INSTALL_DIR/tools/sequoiafs/log

在这两个路径下分别创建以挂载目录别名命名的文件夹。

示例：

本样例中待挂载目录为 /opt/sequoiadb/guestdir/，别名为 guestdir，分别创建配置目录和日志目录，并拷贝一份配置文件样例到配置目录。

```lang-bash
$ alias=guestdir
$ mkdir -p $INSTALL_DIR/tools/sequoiafs/conf/local/$alias/
$ mkdir -p $INSTALL_DIR/tools/sequoiafs/log/$alias/
$ cp $INSTALL_DIR/tools/sequoiafs/conf/sample/sequoiafs.conf $INSTALL_DIR/tools/sequoiafs/conf/local/$alias/
```

配置文件修改参考下面的参数及配置项说明。

##参数及配置项说明##

**--help, -h**

显示帮助信息

**--version, -v**

显示版本信息

**--mountpoint, -m**

挂载目录的全路径

**--alias**

挂载目录的别名，通常为挂载目录全路径的最后一级目录 

**--hosts, -i**

指定需要映射的集合的所属主机节点地址（hostname:svcname），用","分隔多个地址

默认值：localhost:11810

**--username, -u**

数据库用户名

**--passwd, -p**

数据库密码

**--collection, -l**

指定需要映射的目标集合全称，包括集合空间名称和集合名称，例如: mountcs.mountcl

**--metadircollection, -d**

指定目录元数据集合全称，如不指定，则默认根据目标映射集合生成对应集合名称，当目标集合全称为 mountcs.mountcl，默认生成的目录元数据集合全称为 mountcs.mountcl_FS_SYS_DirMeta

**--metafilecollection, -f**

指定文件元数据集合全称，如不指定，则默认根据目标映射集合生成对应集合名称，当目标集合全称为 mountcs.mountcl，默认生成的文件元数据集合全称为 mountcs.mountcl_FS_SYS_FileMeta

**--connectionnum, -n**

指定连接池最大支持连接数大小

取值范围：50~1000

默认值:：100

**--cachesize, -s**

目录LRU缓存大小，单位M

取值范围: 1~200 

默认值: 2

**--confpath, -c**

配置文件路径

**--diaglevel, -g**

设置日志级别

取值范围: 0~5

默认值: 3

**--replsize, -r**

指定元数据集合创建时的ReplSize

取值范围: -1~7

默认值: 2

**--diagnum**

指定日志文件最大个数，-1表示无限制

默认值: 20

**--diagpath**

指定日志文件目录    

**--fuse_allow_other**

FUSE参数，是否允许挂载用户以外的其他用户访问挂载目录

默认值: true

**--fuse_big_writes**

FUSE参数，是否允许超过4KB的写操作，最大32K

默认值: true

**--fuse_max_write**

FUSE参数，指定write请求的最大size

默认值: 131072

**--fuse_max_read**

FUSE参数，指定read请求的最大size

默认值: 131072

**--fuse_nonempty**

FUSE参数，是否允许挂载在非空文件夹上

默认值: false


本章介绍通过 SequoiaFS 挂载目录到 SequoiaDB 数据库的方法。 

##Linux 环境下挂载目录##

当前系统已经安装 SequoiaDB，并已部署 SequoiaDB 集群。

查询 SequoiaDB 安装信息。

```lang-bash
# cat /etc/default/sequoiadb
```
```lang-text
NAME=sdbcm
SDBADMIN_USER=sdbadmin
INSTALL_DIR=/opt/sequoiadb
```

SDBADMIN_USER 为安装用户名，INSTALL_DIR 为安装目录名称。以下操作切换到 SDBADMIN_USER 用户下执行。

```lang-bash
# su sdbadmin
```

在本样例中，待挂载目录为 /opt/sequoiadb/guestdir/ ，挂载目录别名为 guestdir（别名一般为挂载目录全路径的最后一层文件夹名称），挂载目标集合名称为 “mountcs.mountcl”。将这些基本信息分别做如下定义。加载 SequoiaDB 安装信息。

```lang-bash
$ mountpoint=/opt/sequoiadb/guestdir/
$ alias=guestdir
$ collection=mountcs.mountcl
$ . /etc/default/sequoiadb
```

###创建挂载目录及配置文件###

创建待挂载目录。
  
```lang-bash
$ mkdir -p $mountpoint
```

首次挂载前先创建配置文件目录。

```lang-bash
$ mkdir -p $INSTALL_DIR/tools/sequoiafs/conf/local/$alias/
```

复制一份配置样例到配置文件路径。

```lang-bash
$ cp $INSTALL_DIR/tools/sequoiafs/conf/sample/sequoiafs.conf $INSTALL_DIR/tools/sequoiafs/conf/local/$alias/
```

修改配置文件中的待挂载目录、别名、集合名称，其他配置请参考[配置管理](sequoiafs/management/sequoiafsconfig.md)。

```lang-bash
$ FS_ALIAS_CONF=$INSTALL_DIR/tools/sequoiafs/conf/local/$alias/sequoiafs.conf
$ sed -i "s|^mountpoint=|mountpoint=$mountpoint|" $FS_ALIAS_CONF
$ sed -i "s|^alias=|alias=$alias|" $FS_ALIAS_CONF
$ sed -i "s|^collection=|collection=$collection|" $FS_ALIAS_CONF
```

###挂载目录###

使用 fsstart.sh 指定别名挂载目录。

```lang-bash
$ cd $INSTALL_DIR/tools/sequoiafs/bin
$ ./fsstart.sh --alias $alias
```
挂载成功。

```lang-text
/opt/sequoiadb/tools/sequoiafs/bin/sequoiafs -c /opt/sequoiadb/tools/sequoiafs/conf/local/guestdir 
DONE
Total: 1; Succeed: 1; Failed: 0
```

通过 fslist.sh 查看挂载信息。

```lang-bash
$ ./fslist.sh -l
```
```lang-text
Alias     Mountpoint               PID     Collection       ConfPath
guestdir  /opt/sequoiadb/guestdir  129326  mountcs.mountcl  /opt/sequoiadb/tools/sequoiafs/conf/local/guestdir
Total: 1
```
可以看到，目录 /opt/sequoiadb/guestdir 已经挂载。

###验证###

在挂载目录下创建文件 testfile 并写入'hello, this is a testfile!'，创建子目录 testdir。

```lang-bash
$ cd $mountpoint
$ touch testfile
$ echo 'hello, this is a testfile!' >> testfile
$ cat testfile 
```

```lang-text
hello, this is a testfile!
```

```lang-bash
$ mkdir testdir
$ ls
```

```lang-text
testdir  testfile
``` 

##在 Windows 环境下访问挂载目录##

在使用 SequoiaFS 成功完成目录挂载后，可以通过 Samba 服务共享挂载目录，使 Windows 系统上也可以访问该目录。

以下操作在 root 用户下执行。

###Samba 安装###
，执行安装命令。  

在 Centos 和 RedHat 系统，可以通过 yum 进行安装。

```lang-bash
# yum install samba
```

在 Suse 系统，可以通过 zypper 进行安装。

```lang-bash
# zypper install samba
```

在Ubuntu 系统，可以通过 apt 进行安装。

```lang-bash
# apt-get install samba
```

检查当前 Samba 版本，显示出版本号说明已经安装成功。

在 Centos 、RedHat 和 Suse 系统，可以使用rpm 查询版本。

```lang-bash
# rpm -qa samba
```
```lang-text
samba-3.6.23-53.el6_10.x86_64
```

在 Ubuntu 系统中，通过 samba --version 查询版本号。

```lang-bash
# samba --version
```
```lang-text
Version 4.3.11-Ubuntu
```

###Samba 配置###

创建了一个 Linux 用户 sambauser ，将该用户添加到 Samba 用户列表。

```lang-bash
# useradd sambauser
# passwd sambauser
# smbpasswd -a sambauser
```

定义挂载目录，并加载 SequoiaDB 安装信息获取安装用户。

```lang-bash
# mountpoint=/opt/sequoiadb/guestdir/
# . /etc/default/sequoiadb
```

在 Samba 的配置文件 /etc/samba/smb.conf 尾部追加共享目录的信息，其中 path 需根据挂载目录填写，force user 需根据安装用户名称填写。

```lang-bash
# echo [mountpoint] >> /etc/samba/smb.conf
# echo comment = mountpoint >> /etc/samba/smb.conf
# echo path = $mountpoint >> /etc/samba/smb.conf
# echo browseable = Yes >> /etc/samba/smb.conf
# echo guest ok = Yes >> /etc/samba/smb.conf
# echo writable = Yes >> /etc/samba/smb.conf
# echo create mode = 0644 >> /etc/samba/smb.conf
# echo directory mode = 0755 >> /etc/samba/smb.conf
# echo force user = $SDBADMIN_USER >> /etc/samba/smb.conf
```

配置完成后打开 /etc/samba/smb.conf 可以看到尾部增加了如下内容：

```lang-text
[mountpoint]
comment = mountpoint
path = /opt/sequoiadb/guestdir/
browseable = Yes
guest ok = Yes
writable = Yes
create mode = 0644
directory mode = 0755
force user = sdbadmin
```

启动 Samba 服务。

```lang-bash
# service smb start
```

在 ubuntu 系统中启动 samba 使用服务名 smbd。

```lang-bash
# service smbd start
```

###Windows下访问 Samba 共享目录###

Windows 10环境下映射网络驱动，首先选择映射网络驱动器。
![](sequoiafs/samba_windows1.png)  

输入驱动器名称及共享路径 。
![](sequoiafs/samba_windows2.png) 

输入 Samba 用户名密码。
![](sequoiafs/samba_windows3.png) 

在 Windows 下即可通过映射驱动器访问共享目录。
![](sequoiafs/samba_windows4.png) 


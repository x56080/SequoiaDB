在使用 SequoiaFS 成功完成目录挂载后，可以通过 Samba 服务在 Windows 系统上访问该目录。本文讲解在 Ubuntu 系统上安装配置 Samba 服务的流程。   

##Samba 安装##

切换到 root 用户，执行安装命令。  

```lang-bash
$apt install samba
```

检查当前 Samba 版本，显示出版本号说明已经安装成功。

```lang-bash
$samba --version
Version 4.3.11-Ubuntu
```

##Samba 配置##

在 Windows 系统通过 Samba 用户访问 Linux 系统的文件夹，需要有一个 Samba 用户，该用户首先应是一个 Linux 用户。因此需要创建 Linux 用户并将该用户添加为 Samba 用户，也可将已经存在的 Linux 用户添加为 Samba 用户，用户在 Linux 和 Samba 的密码相互独立，可以设置为不同的密码。  
如下样例创建了一个新的 Linux 用户 sambauser ，并将该用户添加为 Samba 用户。

```lang-bash
$useradd sambauser
$passwd sambauser
$smbpasswd -a sambauser
```

假设当前 Linux 系统已经按照[挂载目录](sequoiafs/usage/mount.md)中的指导方式创建了 mountpoint 目录并成功挂载。Samba的主配置文件为 /etc/samba/smb.conf ，在 smb.conf 文件的最下面追加共享目录的配置，该配置控制用户对共享目录的读、写权限。

```
[mount1]
   comment = mount1
   path = /opt/sequoiadb/mountpoint
   browseable = Yes
   read only = Yes
   guest ok = No
```

编辑完成后保存，然后启动 Samba 服务

```lang-bash
$service smbd start
```

如果已经启动过 Samba 服务，则需要重启

```lang-bash
$service smbd restart
```

##SequoiaFS 配置##

SequoiaFS 挂载的目录默认只有执行挂载操作的用户有权限访问，启动 Samba 的 root 用户和其他用户都没有权限访问，如果要支持 Samba 访问需要重新挂载目录并增加 -o allow_other 参数，以便于包括 root 用户在内的其他用户访问该目录。  

首先，在 /etc/fuse.conf 配置中插入一行 user_allow_other   
然后，取消挂载，重新挂载，添加"-o allow_other"参数

```lang-bash
$fusermount -u /opt/sequoiadb/mountpoint
$sequoiafs /opt/sequoiadb/mountpoint -i localhost:11810 -l foo.bar --autocreate -c /opt/sequoiafs/conf/foo_bar/001/ --diagpath  /opt/sequoiafs/log/foo_bar/001/ -o big_writes -o max_write=131072 -o max_read=131072 -o allow_other
```

##Windows下访问 Samba 共享目录##

在 Windows 下访问共享文件夹，首先输入访问 Linux 服务器的IP，如:\\\\192.168.20.69，在弹出的提示框内输入 Samba 用户名和密码，就可以看到 Linux 通过 Samba 共享的目录。  
![](sequoiafs/samba_windows.png)  




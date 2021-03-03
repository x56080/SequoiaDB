本文档主要介绍通过 SequoiaFS 在 SequoiaDB 巨杉数据库挂载目录的方法。
  
##Linux环境下挂载目录##

挂载目录前应确保系统已经安装 SequoiaDB，并已部署 SequoiaDB 集群。

###定义挂载目录基本信息###

1. 查询 SequoiaDB 安装信息（SDBADMIN_USER 为安装用户名，INSTALL_DIR 为安装路径）

  ```lang-bash
  # cat /etc/default/sequoiadb
  ```

  输出结果如下：

  ```lang-text
  NAME=sdbcm
  SDBADMIN_USER=sdbadmin
  INSTALL_DIR=/opt/sequoiadb
  ```

2. 切换到 SDBADMIN_USER 指定用户

  ```lang-bash
  # su sdbadmin
  ```

3. 加载 SequoiaDB 安装信息

  ```lang-bash
  $ . /etc/default/sequoiadb
  ```

4. 定义挂载目录基本信息（挂载目录为 `/home/sdbadmin/guestdir/` ；挂载目录别名为"guestdir"，别名一般为挂载目录全路径的最后一层文件夹名称；挂载目标集合名称为"mountcs.mountcl"）

  ```lang-bash
  $ mountpoint=/home/sdbadmin/guestdir/
  $ alias=guestdir
  $ collection=mountcs.mountcl
  ```

###创建挂载目录及配置文件###

1. 创建挂载目录

  ```lang-bash
  $ mkdir -p $mountpoint
  ```

  首次挂载该目录需要先创建配置文件目录，并复制一份配置样例到配置文件路径

  ```lang-bash
  $ mkdir -p $INSTALL_DIR/tools/sequoiafs/conf/local/$alias/
  $ cp $INSTALL_DIR/tools/sequoiafs/conf/sample/sequoiafs.conf $INSTALL_DIR/tools/sequoiafs/conf/local/$alias/
  ```

2. 修改配置文件中的挂载目录、别名和集合名称，其他配置可参考[配置管理][sequoiafsconfig]

   ```lang-bash
   $ FS_ALIAS_CONF=$INSTALL_DIR/tools/sequoiafs/conf/local/$alias/sequoiafs.conf
   $ sed -i "s|^mountpoint=|mountpoint=$mountpoint|" $FS_ALIAS_CONF
   $ sed -i "s|^alias=|alias=$alias|" $FS_ALIAS_CONF
   $ sed -i "s|^collection=|collection=$collection|" $FS_ALIAS_CONF
   ```

###挂载目录###

1. 使用 `fsstart.sh` 指定别名挂载目录

  ```lang-bash
  $ cd $INSTALL_DIR/tools/sequoiafs/bin
  $ ./fsstart.sh --alias $alias
  ```

  输出结果显示挂载成功：

  ```lang-text
  Start /opt/sequoiadb/tools/sequoiafs/bin/sequoiafs -c /opt/sequoiadb/tools/sequoiafs/conf/local/guestdir --alias guestdir  
  Succeed: 19496
  Total: 1; Succeed: 1; Failed: 0
  ```

2. 通过 `fslist.sh` 查看挂载信息

  ```lang-bash
  $ ./fslist.sh -l
  ```

  输出结果如下：

  ```lang-text
  Alias     Mountpoint               PID    Collection       ConfPath
  guestdir  /home/sdbadmin/guestdir  19496  mountcs.mountcl  /opt/sequoiadb/tools/sequoiafs/conf/local/guestdir
  Total: 1
  ```

###验证###

1. 进入 $mountpoint 指定目录

  ```lang-bash
  $ cd $mountpoint
  ```

2. 在挂载目录下创建文件 `testfile` 并写入"hello, this is a testfile!"

  ```lang-bash
  $ touch testfile
  $ echo 'hello, this is a testfile!' >> testfile
  ```

3. 查看 `testfile` 文件内容是否写入

  ```lang-bash
  $ cat testfile 
  ```

4. 创建子目录 `testdir`

  ```lang-bash
  $ mkdir testdir
  ```

5. 查看目录是否创建成功

  ```lang-bash
  $ ls
  ``` 

##在Windows环境下访问挂载目录##

用户在使用 SequoiaFS 成功完成目录挂载后，可以通过 Samba 服务共享挂载目录，使 Windows 系统上也可以访问该目录。

以下操作均在 root 用户下执行。

###Samba 安装###

* 对于 CentOS/Red Hat：

   ```lang-bash
   # yum install samba
   ```

* 对于 SUSE：

   ```lang-bash
   # zypper install samba
   ```

* 对于 Ubuntu：

   ```lang-bash
   # apt-get install samba
   ```

###检查当前 Samba 版本###

* 对于 CentOS/Red Hat/SUSE：

   ```lang-bash
   # rpm -qa samba
   ```

   输出当前 Samba 版本号

   ```lang-text
   samba-3.6.23-53.el6_10.x86_64
   ```

* 对于 Ubuntu：

   ```lang-bash
   # samba --version
   ```

   输出当前 Samba 版本号

   ```lang-text
   Version 4.3.11-Ubuntu
   ```

###Samba 配置###

1. 创建一个 Linux 用户 `sambauser` 

   ```lang-bash
   # useradd sambauser
   ```
2. 为 `sambauser` 用户设置密码

   ```lang-bash
   # passwd sambauser
   ```

   根据提示设置密码

   ```lang-text
   Enter new UNIX password: 
   Retype new UNIX password: 
   passwd: password updated successfully
   ```

3. 将该用户添加到 Samba 用户列表

   ```lang-bash
   # smbpasswd -a sambauser
   ```

   根据提示设置密码

   ```lang-text
   New SMB password:
   Retype new SMB password:
   Added user sambauser.
   ```

4. 定义挂载目录，并加载 SequoiaDB 安装信息获取安装用户

   ```lang-bash
   # mountpoint=/home/sdbadmin/guestdir/
   # . /etc/default/sequoiadb
   ```

5. 在 Samba 的配置文件 `/etc/samba/smb.conf` 尾部追加共享目录的信息，其中 path 需根据挂载目录填写，force user 需根据安装用户名称填写

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

6. 启动 Samba 服务

   ```lang-bash
   # service smb start
   ```

   对于 ubuntu 系统，需要使用服务名 smbd 启动 Samba

   ```lang-bash
   # service smbd start
   ```

###Windows下访问 Samba 共享目录###

1. Windows 10 环境下打开【此电脑】，选择【映射网络驱动器】
![windows1][samba_windows1]  

2. 在【驱动器】中输入本地映射驱动器名称，在【文件夹】中输入共享路径
![windows2][samba_windows2] 

3. 输入 Samba 用户名密码
![windows3][samba_windows3] 

4. 在 Windows 下即可通过映射驱动器访问共享目录
![windows4][samba_windows4] 



[^_^]:
     本文使用的所有链接及引用
[sequoiafsconfig]:manual/Database_Instance/Object_Instance/File_Instance/Management/sequoiafsconfig.md
[samba_windows1]:images/Database_Instance/Object_Instance/File_Instance/Operation/samba_windows1.png
[samba_windows2]:images/Database_Instance/Object_Instance/File_Instance/Operation/samba_windows2.png
[samba_windows3]:images/Database_Instance/Object_Instance/File_Instance/Operation/samba_windows3.png
[samba_windows4]:images/Database_Instance/Object_Instance/File_Instance/Operation/samba_windows4.png

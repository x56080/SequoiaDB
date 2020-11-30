本文档主要介绍 FUSE 和 SequoiaFS 的安装部署。

##检查FUSE版本##

1. 查看是否安装 FUSE

   ```lang-bash
   # which fusermount
   ```

2. 查看 FUSE 版本号

   ```lang-bash
   # fusermount --version
   ```

> **Note：**
>
> 若未安装 FUSE 或 FUSE 版本号低于 2.8.6，需进行 [FUSE 安装][install_deploy]。

##FUSE安装##

###安装说明###

* 安装 SequoiaFS 之前应确保已安装 SequoiaDB 巨杉数据库；
* 需要使用 root 用户权限进行安装部署，应确保 root 用户对相关命令或配置文件具有访问权限；
* CentOS 7、Red Hat 7、SUSE 11.3 和 Ubuntu 14 及其以上版本的操作系统可参考包管理器安装，其他系统可参考源码安装或尝试其他方式自行安装。

###安装步骤###

**包管理器安装**

* 对于 CentOS 7/Red Hat 7 及其更高版本系统：

   ```lang-bash
   # yum install fuse
   ```

* 对于 SUSE 11.3 及其更高版本系统：

   ```lang-bash
   # zypper install fuse
   ```

* 对于 Ubuntu 14 及其更高版本系统：

   ```lang-bash
   # apt-get install fuse
   ```

**源码安装**

用户自行下载 libfuse 的源码包 [libfuse-fuse-2.8.6.tar.gz][jar] 进行编译安装。

1. 解压源码包并进入源码包目录

   ```lang-bash
   # tar -xzvf libfuse-fuse_2_8_6.tar.gz  
   # cd libfuse-fuse_2_8_6/
   ```

2. 编译安装 libfuse 库，需要通过 --prefix 参数指定安装路径

   ```lang-bash
   # ./makeconf.sh  
   # ./configure --prefix=/opt/sequoiadb/fuse  
   # make   
   # make install
   ```  

3. 查询安装后的版本号

   ```lang-bash
   # /opt/sequoiadb/fuse/bin/fusermount --version
   ```

4. 将 FUSE 可执行程序路径配置到数据库安装用户的 PATH 环境变量中（配置路径必须与 --prefix 参数指定路径一致）

   ```lang-bash
   # . /etc/default/sequoiadb
   # echo 'export PATH="/opt/sequoiadb/fuse/bin:$PATH"' >> /home/$SDBADMIN_USER/.bashrc
   ```

##FUSE配置##

在 `/etc/fuse.conf` 添加配置"user_allow_other"

```lang-bash
# echo "user_allow_other" >> /etc/fuse.conf
```


[^_^]:
     本文使用的所有链接及引用
[install_deploy]:manual/Database_Instance/Object_Instance/File_Instance/install_deploy.md#FUSE安装
[jar]:https://github.com/libfuse/libfuse/archive/fuse_2_8_6.tar.gz
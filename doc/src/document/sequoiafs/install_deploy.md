本章介绍 Fuse 和 SequoiaFS 的安装部署方法。

确保已安装 SequoiaDB，若未安装，参考[数据库安装](installation/deployment/command_installation/installation.md) 进行安装。

##安装 fuse##

以下操作在 root 用户下执行。

###检查 fuse 库版本###
查看是否安装了 fuse。

```lang-bash
# which fusermount
```

```lang-text
/bin/fusermount
```
查看 fuse 版本号。

```lang-bash
# fusermount --version
```

```lang-text
fusermount version: 2.8.6
```    

如果未安装 fuse 或者 fuse 版本号低于2.8.6，请参考以下步骤进行 libfuse 的安装。

###包管理器安装###

在 CentOS、RedHat、SuSE、Ubuntu 可以参考如下指导进行安装，其他系统请参考源码安装或尝试其他方式自行安装。

- CentOS 和 RedHat 

CentOS 7 和 RedHat 7 及更高版本，可以使用 yum 安装，低版本请参考源码安装。

```lang-bash
# yum install fuse
```

- SuSE 

SuSE11SP3 及更高版本，可以使用 zypper 安装，低版本请参考源码安装。

```lang-bash
# zypper install fuse
```

- Ubuntu

Ubuntu 14 及更高版本系统，可以通过 apt 进行安装，低版本请参考源码安装。

```lang-bash
# apt-get install fuse
```

###源码安装###
下载 libfuse 的源码包自行编译安装   [fuse-2.8.6.tar.gz](https://github.com/libfuse/libfuse/archive/fuse_2_8_6.tar.gz)     
解压源码包，编译安装，通过 --prefix 参数指定安装路径为 /opt/sequoiadb/fuse。    
  
```lang-bash
# tar -xzvf libfuse-fuse_2_8_6.tar.gz  
# cd libfuse-fuse_2_8_6/
# ./makeconf.sh  
# ./configure --prefix=/opt/sequoiadb/fuse  
# make   
# make install
```  

查询安装后的版本号。

```lang-bash
# /opt/sequoiadb/fuse/bin/fusermount --version
```

得到当前安装版本为2.8.6

```lang-text
fusermount version: 2.8.6
```

将 fuse 可执行程序路径配置到数据库安装用户的 PATH 环境变量中。

```lang-bash
# . /etc/default/sequoiadb
# echo 'export PATH="/opt/sequoiadb/fuse/bin:$PATH"' >> /home/$SDBADMIN_USER/.bashrc
```

###fuse 配置###

在 /etc/fuse.conf 中增加一行 user_allow_other。      

```lang-bash
# echo "user_allow_other" >> /etc/fuse.conf
```


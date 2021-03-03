在安装 SequoiaDB 产品之前，请确保您选择的系统满足必须的操作系统，硬件，通信，磁盘和内存的要求。

##硬件要求##

| 需求项 | 最低配置                                                             | 推荐配置                                                              |
|--------|----------------------------------------------------------------------|-----------------------------------------------------------------------|
| CPU    |  x64（64 位 AMD64 或 Intel EM64T 处理器）                            | x64（64 位 AMD64 或 Intel EM64T 处理器），8 核或以上                  |
| 磁盘   | 至少 20GB 可用空间                                                   | 500GB 或以上可用空间                                                  |
| 内存   | 至少 1GB 物理内存                                                    | 32GB 或以上物理内存                                                   |
| 网卡   | 至少 1 张 1000Mb/s 速率网卡                                          | 10000Mb/s 速率网卡                                                    |


##受支持的操作系统##


| 系统类型       | 系统列表                                                   |
|----------------|------------------------------------------------------------|
| Linux          |- Red Hat Enterprise Linux (RHEL) 6<br> - Red Hat Enterprise Linux (RHEL) 7<br> - SUSE Linux Enterprise Server (SLES) 11 Service Pack 1 <br>   - SUSE Linux Enterprise Server (SLES) 11 Service Pack 2 <br> 	- SUSE Linux Enterprise Server (SLES) 12 Service Pack 1 <br> 	- Ubuntu 12.x <br> - Ubuntu 14.x <br> - Ubuntu 16.x <br> - CentOS 6.x <br> - CentOS 7.x <br>                                    |

注意：未在上述列表中列举的 Linux 操作系统并不代表不能用于安装 SequoiaDB。当将这些 Linux 操作系统应用于生产环境时，建议联系 SequoiaDB 技术支持，以获得更详细的信息。


##软件要求##

###Linux 系统要求###

在安装 SequoiaDB 之前，应该先对 Linux 系统相关的配置进行检查和设置。需要检查和设置的配置包括：

 * 配置主机名

 * 配置主机名/IP地址映射

 * 配置防火墙

####配置主机名####

- **配置方法**

  - 对于SUSE:
     1. 使用 root 权限登陆，执行 hostname sdbserver1 （sdbserver1为主机名称，可根据需要修改。）；

         ```lang-javascript
         $ hostname sdbserver1
         ```
     2. 打开 /etc/HOSTNAME 文件；

         ```lang-javascript
         $ vi /etc/HOSTNAME
         ```
     3. 修改文件内容，配置为主机名称 sdbserver1 （主机名称）；

         ```
         sdbserver1
         ```
     4. 按 : wq 保存退出；

  - 对于 RedHat：
     1. 使用 root 权限登陆，执行 hostname sdbserver1 （sdbserver1为主机名称，可根据需要修改。）；

         ```lang-javascript
         $ hostname sdbserver1
         ```
     2. RedHat7 以下的系统，打开 /etc/sysconfig/network 文件；

         ```lang-javascript
         $ vi /etc/sysconfig/network
         ```

         如果是 RedHat7 系统，则打开 /etc/hostname 文件：

         ```lang-javascript
         $ vi /etc/hostname
         ```
     3. 将 HOSTNAME 一行修改为 HOSTNAME = sdbserver1 （其中sdbserver1 为新主机名）；

         ```
         HOSTNAME = sdbserver1
         ```
     4. 按 : wq 保存退出；

  - 对于 Ubuntu：
     1. 使用 root 权限登陆，执行 hostname sdbserver1 （sdbserver1为主机名称，可根据需要修改。）；

         ```lang-javascript
             $ hostname sdbserver1
             ```
     2. 打开 /etc/hostname 文件；

         ```lang-javascript
         $ vi /etc/hostname
         ```
     3. 修改文件内容，配置为主机名称: sdbserver1

         ```
         sdbserver1
         ```
     4. 按 : wq 保存退出；

- **验证方法**
  执行 hostname 命令，确认打印信息是否为 “sdbserver1”

  ```lang-javascript
  $ hostname
  ```

####配置主机名/IP地址映射####

- **配置方法**

  	1. 使用 root 权限，打开 /etc/hosts 文件

     	```lang-javascript
     	$ vi /etc/hosts
     	```
  	2. 修改 /etc/hosts ，将服务器节点的主机名与IP映射关系配置到该文件中

     	```
     	192.168.20.200 sdbserver1
     	192.168.20.201 sdbserver2
     	192.168.20.202 sdbserver3
     	```

  	3. 保存退出

- **验证方法**
  1. ping sdbserver1（本机主机名） 可以 ping 通

     ```lang-javascript
     $ ping sdbserver1
     ```
  2. ping sdbserver2（远端主机名） 可以 ping 通

     ```lang-javascript
     $ ping sdbserver2
     ```

####关闭防火墙 (需要管理员权限)####

- **配置方法**

  - 对于 SUSE 11，执行如下命令：

     ```lang-bash
     # SuSEfirewall2 stop    # 临时关闭防火墙
     # chkconfig SuSEfirewall2_init off    # 设置开机禁用防火墙
     # chkconfig SuSEfirewall2_setup off
	 ```

  - 对于 SUSE 12，执行如下命令：

     ```lang-bash
     # systemctl stop SuSEfirewall2.service    # 临时关闭防火墙
     # systemctl disable SuSEfirewall2.service    # 设置开机禁用防火墙
	 ```

  - 对于 Red Hat 6/CentOS 6 及以下系统：

	 执行如下命令

     ```lang-bash
     # service iptables stop    # 临时关闭防火墙
     # chkconfig iptables off    # 设置开机禁用防火墙
     ```
  - 对于 Red Hat 7/Red Hat 8 和 CentOS 7/CentOS 8：

	 执行如下命令

     ```lang-bash
     # systemctl stop firewalld.service    # 临时关闭防火墙
     # systemctl disable firewalld.service    # 设置开机禁用防火墙
     ```
  - 对于 Ubuntu：

     执行如下命令

     ```lang-bash
     # ufw disable
     ```

- **验证方法**

  - 对于 SUSE 11：
 
     执行命令，若打印以下信息，说明关闭防火墙成功

     ```lang-bash
     # chkconfig -list | grep fire
     SuSEfirewall2_init       	0:off	1:off	2:off	3:off	4:off	5:off	6:off
     SuSEfirewall2_setup      	0:off	1:off	2:off	3:off	4:off	5:off	6:off
     ```

  - 对于 SUSE 12：

     执行命令，若打印以下信息，说明关闭防火墙成功

     ```lang-bash
     # systemctl status SuSEfirewall2.service
     ● SuSEfirewall2.service - SuSEfirewall2 phase 2
           Loaded: loaded (/usr/lib/systemd/system/SuSEfirewall2.service; disabled; vendor preset: disabled)
           Active: inactive (dead)
     ```

  - 对于 Red Hat 6/CentOS 6 及以下系统：

     执行命令，若打印以下信息，说明关闭防火墙成功

     ```lang-bash
     # chkconfig --list iptables
     iptables       	0:off	1:off	2:off	3:off	4:off	5:off	6:off
     ```
  - 对于 Red Hat 7/Red Hat 8 和 CentOS 7/CentOS 8：

     执行命令，若打印以下信息，说明关闭防火墙成功

     ```lang-bash
     # systemctl status firewalld.service
     ● firewalld.service - firewalld - dynamic firewall daemon
           Loaded: loaded (/usr/lib/systemd/system/firewalld.service; disabled; vendor preset: enabled)
           Active: inactive (dead)
             Docs: man:firewalld(1)
     ```
  - 对于 Ubuntu：

     执行命令，若打印以下信息，说明关闭防火墙成功

     ```lang-bash
     # ufw status
     Status: inactive
     ```

>**Note:**
>1. 以上“配置主机名”、“配置主机名/IP地址映射”和“配置防火墙”这几个步骤都需要在每台作为数据库服务器的机器上配置;
>2. 社区版要求系统安装glibc 2.15以及libstdc++ 6.0.18以上版本。
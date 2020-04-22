在安装 SequoiaDB 巨杉数据库产品之前，需确保所选择的系统满足产品对操作系统、硬件、通信、磁盘和内存的要求。

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
| Linux          |- Red Hat Enterprise Linux (RHEL) 6<br> - Red Hat Enterprise Linux (RHEL) 7<br> - Red Hat Enterprise Linux (RHEL) 8<br>- SUSE Linux Enterprise Server (SLES) 11 Service Pack 1 <br>   - SUSE Linux Enterprise Server (SLES) 11 Service Pack 2 <br> 	- SUSE Linux Enterprise Server (SLES) 12 Service Pack 1 <br> 	- Ubuntu 12.x <br> - Ubuntu 14.x <br> - Ubuntu 16.x <br> - CentOS 6.x <br> - CentOS 7.x <br> - CentOS 8.x <br>                                   |

> **Note：**
>
>* 未列举在上述列表中的 Linux 操作系统并不代表不能用于安装 SequoiaDB。
>
>* 需要将这些 Linux 操作系统应用于生产环境时，建议联系 SequoiaDB 技术支持，以获得更详细的信息。


##软件要求##

###Linux 系统要求###

在安装 SequoiaDB 之前，应该先对 Linux 系统相关的配置进行检查和设置。需要检查和设置的配置包括：

 * 配置主机名

 * 配置主机名/IP地址映射

 * 配置防火墙

 * 配置 SELinux

配置说明：

* 需要使用 root 用户权限进行配置，应确保 root 用户对相关命令或配置文件具有访问权限；

* 示例中"sdbserver1"为主机名称，用户可以根据需要修改该主机名。

####配置主机名####

- **配置方法**

  - 对于SUSE:
     1. 设置主机名

         ```lang-bash
         # hostname sdbserver1
         ```
     2. 将主机名持久化到配置文件

         ```lang-bash
         # echo "sdbserver1" > /etc/HOSTNAME
         ```

  - 对于 Red Hat 6/CentOS 6 及以下的系统：
     1. 设置主机名

         ```lang-bash
         # hostname sdbserver1
         ```
     2. 将主机名持久化到配置文件

         ```lang-bash
         # sed -i "s/HOSTNAME=.*/HOSTNAME=sdbserver1/g" /etc/sysconfig/network
         ```

  - 对于 Red Hat 7/Red Hat 8 和 CentOS 7/CentOS 8：
     1. 设置主机名

         ```lang-bash
         # hostname sdbserver1
         ```
     2. 将主机名持久化到配置文件

         ```lang-bash
         # echo "sdbserver1" > /etc/hostname
         ```

  - 对于 Ubuntu：
     1. 设置主机名

         ```lang-bash
         # hostname sdbserver1
         ```
     2. 将主机名持久化到配置文件

         ```lang-bash
         # echo "sdbserver1" > /etc/hostname
         ```

- **验证方法**

  执行 hostname 命令，若打印信息是为 “sdbserver1”，说明配置主机名成功

  ```lang-bash
  # hostname
  sdbserver1
  ```

####配置主机名IP地址映射####

- **配置方法**

  	将服务器节点的主机名与IP映射关系配置到 /etc/hosts 文件中

     ```lang-bash
     # echo "192.168.20.200 sdbserver1" >> /etc/hosts
     # echo "192.168.20.201 sdbserver2" >> /etc/hosts
     ```

- **验证方法**
  1. ping sdbserver1（本机主机名） 可以 ping 通

     ```lang-bash
     # ping sdbserver1
     ```
  2. ping sdbserver2（远端主机名） 可以 ping 通

     ```lang-bash
     # ping sdbserver2
     ```

####关闭防火墙####

- **配置方法**

  - 对于 SUSE：

     	执行如下命令

         ```lang-bash
         # SuSEfirewall2 stop
         # chkconfig SuSEfirewall2_init off
         # chkconfig SuSEfirewall2_setup off
	       ```

  - 对于 Red Hat 6/CentOS 6 及以下的系统：

		执行如下命令

         ```lang-bash
         # service iptables stop
         # chkconfig iptables off
         ```

  - 对于 Red Hat 7/Red Hat 8 和 CentOS 7/CentOS 8：

		执行如下命令

         ```lang-bash
         # systemctl stop firewalld.service
         # systemctl disable firewalld.service
         ```

  - 对于 Ubuntu：

     	执行如下命令

         ```lang-bash
         # ufw disable
         ```

- **验证方法**

  - 对于 SUSE：

     执行命令，若打印以下信息，说明关闭防火墙成功

     ```lang-bash
     # chkconfig -list | grep fire
     SuSEfirewall2_init       	0:off	1:off	2:off	3:off	4:off	5:off	6:off
     SuSEfirewall2_setup      	0:off	1:off	2:off	3:off	4:off	5:off	6:off
     ```

  - 对于 Red Hat 6/CentOS 6 及以下的系统：

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
           Loaded: loaded(/usr/lib/systemd/system/firewalld.service; disabled; vendor preset: enabled)
           Active: inactive (dead)
             Docs: man:firewalld(1)
      ```

  - 对于 Ubuntu:

     执行命令，若打印以下信息，说明关闭防火墙成功

     ```lang-bash
     # ufw status
     Status: inactive
     ```

#### 配置 SELinux ####

针对 SELinux 可以配置为关闭或者将模式调整成 permissive，建议关闭 SELinux。

- **关闭 SELinux**

  - 配置方法

   1. 修改配置文件，将 SELINUX 配置为 disabled

     ```lang-bash
     # sed -i "s/SELINUX=.*/SELINUX=disabled/g" /etc/selinux/config
     ```

   2. 重启操作系统

     ```lang-bash
     # reboot # 需要重启系统
     ```

  - 验证方法

     ```lang-bash
     # sestatus
     SELinux status:                 disabled
     ```

- **模式设置成 permissive**

  - 配置方法

   1. 关闭 SELinux 防火墙

     ```lang-bash
     # setenforce 0
     ```

   2. 修改配置文件，将 SELINUX 配置为 permissive

     ```lang-bash
    # sed -i "s/SELINUX=.*/SELINUX=permissive/g" /etc/selinux/config
     ```

  - 验证方法

     ```lang-bash
     # sestatus
     SELinux status:                 enabled
     SELinuxfs mount:                /sys/fs/selinux
     SELinux root directory:         /etc/selinux
     Loaded policy name:             targeted
     Current mode:                   permissive
     Mode from config file:          permissive
     Policy MLS status:              enabled
     Policy deny_unknown status:     allowed
     Max kernel policy version:      28
     ```

>**Note:**
>
>* 主机名、主机名/IP地址映射、防火墙和 SELinux 需要在每台物理机器上进行配置；
>
>* 社区版要求系统安装 glibc 2.15 以及 libstdc++ 6.0.18 以上版本。
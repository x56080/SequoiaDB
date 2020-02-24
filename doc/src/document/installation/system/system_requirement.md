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

 * 配置 SELinux

####配置主机名####

- **配置方法**

  - 对于SUSE:
     1. 使用 root 权限登陆，执行 hostname sdbserver1 （sdbserver1为主机名称，可根据需要修改。）；

         ```lang-bash
         $ hostname sdbserver1
         ```
     2. 打开 /etc/HOSTNAME 文件；

         ```lang-bash
         $ vi /etc/HOSTNAME
         ```
     3. 修改文件内容，配置为主机名称 sdbserver1 （主机名称）；

         ```lang-ini
         sdbserver1
         ```
     4. 按 : wq 保存退出；

  - 对于 RedHat：
     1. 使用 root 权限登陆，执行 hostname sdbserver1 （sdbserver1为主机名称，可根据需要修改。）；

         ```lang-bash
         $ hostname sdbserver1
         ```
     2. RedHat7 以下的系统，打开 /etc/sysconfig/network 文件；

         ```lang-bash
         $ vi /etc/sysconfig/network
         ```

         如果是 RedHat7 系统，则打开 /etc/hostname 文件：

         ```lang-bash
         $ vi /etc/hostname
         ```

     3. 将 HOSTNAME 一行修改为 HOSTNAME = sdbserver1 （其中sdbserver1 为新主机名）；

         ```lang-ini
         HOSTNAME = sdbserver1
         ```
     4. 按 : wq 保存退出；

  - 对于 Ubuntu：
     1. 使用 root 权限登陆，执行 hostname sdbserver1 （sdbserver1为主机名称，可根据需要修改。）；

         ```lang-bash
             $ hostname sdbserver1
             ```
     2. 打开 /etc/hostname 文件；

         ```lang-bash
         $ vi /etc/hostname
         ```
     3. 修改文件内容，配置为主机名称: sdbserver1

         ```lang-ini
         sdbserver1
         ```
     4. 按 : wq 保存退出；

- **验证方法**
  执行 hostname 命令，确认打印信息是否为 “sdbserver1”

  ```lang-bash
  $ hostname
  ```

####配置主机名IP地址映射####

- **配置方法**

  	1. 使用 root 权限，打开 /etc/hosts 文件

     	```lang-bash
     	$ vi /etc/hosts
     	```
  	2. 修改 /etc/hosts ，将服务器节点的主机名与IP映射关系配置到该文件中

     	```lang-ini
     	192.168.20.200 sdbserver1
     	192.168.20.201 sdbserver2
     	192.168.20.202 sdbserver3
     	```

  	3. 保存退出

- **验证方法**
  1. ping sdbserver1（本机主机名） 可以 ping 通

     ```lang-bash
     $ ping sdbserver1
     ```
  2. ping sdbserver2（远端主机名） 可以 ping 通

     ```lang-bash
     $ ping sdbserver2
     ```

####关闭防火墙 (需要管理员权限)####

- **配置方法**

  - 对于 SUSE:

     	执行如下命令

         ```lang-bash
         $ SuSEfirewall2 stop
         $ chkconfig SuSEfirewall2_init off
         $ chkconfig SuSEfirewall2_setup off
	       ```

  - 对于 RedHat：

		执行如下命令

         ```lang-bash
         $ service iptables stop
         $ chkconfig iptables off
         ```
  - 对于 Ubuntu：

     	执行如下命令

         ```lang-bash
         $ ufw disable
         ```

- **验证方法**

  - 对于 SUSE

     ```lang-bash
     $ chkconfig -list | grep fire
     ```

  - 对于 RedHat:

     ```lang-bash
     $ service iptables status
     ```

  - 对于 Ubuntu:

     ```lang-bash
     $ ufw status
     ```

>**Note:**
>1. 以上“配置主机名”、“配置主机名/IP地址映射”和“配置防火墙”这几个步骤都需要在每台作为数据库服务器的机器上配置;
>2. 社区版要求系统安装glibc 2.15以及libstdc++ 6.0.18以上版本。

#### SELinux 配置 ####

针对 SELinux 可以配置为关闭或者将模式调整成 permissive，建议关闭 SELinux。

- **关闭 SELinux**

 - 配置方法

   1. 使用 root 权限，打开 /etc/selinux/config 文件

     ```lang-bash
     $ vi /etc/selinux/config
     ```

   2. 将 SELINUX 调整成 disabled

     ```lang-ini
     # This file controls the state of SELinux on the system.
     # SELINUX= can take one of these three values:
     #     enforcing - SELinux security policy is enforced.
     #     permissive - SELinux prints warnings instead of enforcing.
     #     disabled - No SELinux policy is loaded.
     SELINUX=disabled
     # SELINUX=enforcing
     # SELINUXTYPE= can take one of three two values:
     #     targeted - Targeted processes are protected,
     #     minimum - Modification of targeted policy. Only selected processes are protected.
     #     mls - Multi Level Security protection.
     SELINUXTYPE=targeted
     ```

   3. 重启操作系统

     ```lang-bash
     $ reboot # 需要重启系统
     ```

  - 验证方法

     ```lang-bash
     $ sestatus
     SELinux status:                 disabled
     ```

- **模式设置成 permissive**

  - 配置方法

   1. 使用 root 权限，执行命令 setenforce 0 ; 打开 /etc/selinux/config 文件

     ```lang-bash
     $ setenforce 0
     $ vi /etc/selinux/config
     ```

   2. 调整 SELINUX 的值为 permissive

     ```lang-ini
     # This file controls the state of SELinux on the system.
     # SELINUX= can take one of these three values:
     #     enforcing - SELinux security policy is enforced.
     #     permissive - SELinux prints warnings instead of enforcing.
     #     disabled - No SELinux policy is loaded.
     SELINUX=permissive
     # SELINUX=enforcing
     # SELINUXTYPE= can take one of three two values:
     #     targeted - Targeted processes are protected,
     #     minimum - Modification of targeted policy. Only selected processes are protected.
     #     mls - Multi Level Security protection.
     SELINUXTYPE=targeted
     ```

  - 验证方法

     ```lang-bash
     $ sestatus
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

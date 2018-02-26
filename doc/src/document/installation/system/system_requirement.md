在安装 SequoiaDB 产品之前，请确保您选择的系统满足必须的操作系统，硬件，通信，磁盘和内存的要求。

##硬件要求##

| 需求项 | 要求                                                                 | 建议                                                                  |
|--------|----------------------------------------------------------------------|-----------------------------------------------------------------------|
| CPU    |  -   x86（Intel Pentium、Intel Xeon 和 AMD）32位 Intel 和 AMD 处理器<br>-   x64（64位 AMD64 和 Intel EM64T 处理器）<br> -   PowerPC 7 或者 PowerPC 7+ 处理器                                                                     | 建议采用 X64（64 位 AMD64 和 Intel EM64T 处理器 ）或者 PowerPC 处理器 |
| 磁盘   | 至少 10GB 空间                                                       | 建议大于 100GB 磁盘空间                                               |
| 内存   | 至少 1GB                                                             | 大于 2GB 物理内存                                                     |
| 网卡   | 配备至少 1 张网卡                                                    | 建议至少配置 1GE 网卡                                                 |


##受支持的操作系统##

| 系统类型       | 系统列表                                                   |
|----------------|------------------------------------------------------------|
| Linux          |- Red Hat Enterprise Linux (RHEL) 6<br> - SUSE Linux Enterprise Server (SLES) 11 Service Pack 1 <br>   - SUSE Linux Enterprise Server (SLES) 11 Service Pack 2 <br> 	- SUSE Linux Enterprise Server (SLES) 12 Service Pack 1 <br> 	- Ubuntu 12 <br> 	- CentOS 6                                    |
| Power PC Linux |  - Red Hat Enterprise Linux (RHEL) 6 <br>  - SUSE Linux Enterprise Server (SLES)11 Service Pack 1  <br>  - SUSE Linux Enterprise Server (SLES)11 Service Pack 2  |

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
     2. 打开 /etc/sysconfig/network 文件；  
         
         ```lang-javascript
         $ vi /etc/sysconfig/network
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
  - 使用 root 权限，打开 /etc/hosts 文件 
   
     ```lang-javascript
     $ vi /etc/hosts
     ```
  - 修改 /etc/hosts ，将服务器节点的主机名与IP映射关系配置到该文件中  

     ```
     192.168.20.200 sdbserver1  
     192.168.20.201 sdbserver2  
     192.168.20.202 sdbserver3
     ```
  - 保存退出

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

  - 对于 SUSE:   
     1. 执行如下命令
         
         ```lang-javascript
         $ SuSEfirewall2 stop
         $ chkconfig SuSEfirewall2_init off
         $ chkconfig SuSEfirewall2_setup off
	       ```

  - 对于 RedHat：
     1. 执行如下命令    

         ```lang-javascript
         $ service iptables stop
         $ chkconfig iptables off
         ```
  - 对于 Ubuntu： 
     1. 执行如下命令

         ```lang-javascript
         $ ufw disable
         ```

- **验证方法**
  - 对于 SUSE

     ```lang-javascript
     $ chkconfig -list | grep fire
     ``` 
  - 对于 RedHat:
     
     ```lang-javascript
     $ service iptables status
     ``` 
  - 对于 Ubuntu:
     
     ```lang-javascript
     $ ufw status
     ``` 

>**Note:**  
>1. 以上“配置主机名”、“配置主机名/IP地址映射”和“配置防火墙”这几个步骤都需要在每台作为数据库服务器的机器上配置;  
>2. 社区版要求系统安装glibc 2.15以及libstdc++ 6.0.18以上版本。
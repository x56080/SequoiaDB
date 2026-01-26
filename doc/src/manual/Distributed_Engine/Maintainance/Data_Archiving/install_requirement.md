## 硬件平台

| 硬件平台类型        | 硬件平台列表          |
|---------------|-----------------|
| x86 架构        | 通用 x86 硬件平台     |
| ARM64 架构      | 通用 ARM64 硬件平台       |

## 操作系统

64 位操作系统

## 服务器配置

- **主机名命名，需要遵循 Java 软件体系的主机名规范。** 合法主机名，如：redhat75-server1-sdb、redhat-75.server1.sdb。非法主机名，如：redhat75_server1_sdb（不能含有下划线，建议使用连字符：- 代替）具体规则如下
    1. 主机名由数字（0-9）、字母（a-z、A-Z）、连字符（-）、点字符（.）组成
    2. 不能以连字符、点字符开头和结尾。如：-server-sdb、server1.sdb.
    3. 连字符和点字符不能连续出现。如：-- .. -. .-
    4. 如果存在点字符，最后一个点字符之后的所有字符不能含有非字母字符。如：redhat-75.server1.sdb1、redhat-75.server1.sc-m

- 配置 /etc/hosts 文件，建立主机名和 IP 地址之间的映射关系，确保每台主机能够通过主机名访问其他主机。

- 最小内存要求：8 G

## 软件要求

### 中间件版本要求

| 需求项       | 版本要求        |
|-----------|-------------|
| JDK       | 1.8 及以上版本   |
| SequoiaDB | 5.8.6 及以上版本 |

## 操作指南

以 Linux 操作系统为例，要求如下：

### 配置主机名

- **配置方法**

    - 对于SUSE:
        1. 使用 root 权限登陆，执行 hostname server1 （server1为主机名称，可根据需要修改。）；

            ```lang-javascript
            # hostname server1
            ```
        2. 打开 /etc/HOSTNAME 文件；

            ```lang-javascript
            # vi /etc/HOSTNAME
            ```
        3. 修改文件内容，配置为主机名称 server1 （主机名称）；

            ``` 
            server1
            ```
        4. 按 : wq 保存退出；

    - 对于 RedHat：
        1. 使用 root 权限登陆，执行 hostname server1 （server1为主机名称，可根据需要修改。）；

            ```lang-javascript
            # hostname server1
            ```
        2. 打开 /etc/sysconfig/network 文件；

            ```lang-javascript
            # vi /etc/sysconfig/network
            ```
        3. 将 HOSTNAME 一行修改为 HOSTNAME = server1 （其中server1 为新主机名）；

            ```
            HOSTNAME = server1 
            ```
        4. 按 : wq 保存退出；

    - 对于 Ubuntu：
        1. 使用 root 权限登陆，执行 hostname server1 （server1为主机名称，可根据需要修改。）；

            ```lang-javascript
            # hostname server1
            ```
        2. 打开 /etc/hostname 文件；

            ```lang-javascript
            # vi /etc/hostname
            ```
        3. 修改文件内容，配置为主机名称 server1

            ```
            server1
            ```
        4. 执行 : wq 保存退出；

- **验证方法**

  执行 hostname 命令，确认打印信息是否为 “server1”

  ```lang-javascript
  # hostname
  ```

### 配置主机名/ip地址映射

- **配置方法**
    - 使用 root 权限，打开 /etc/hosts 文件

       ```lang-javascript
       # vi /etc/hosts
       ```
    - 修改 /etc/hosts ，将服务器节点的主机名与IP映射关系配置到该文件中

       ```
       192.168.20.200 server1  
       192.168.20.201 server2  
       192.168.20.202 server3
       ```
    - 保存退出

- **验证方法**
    1. ping server1（本机主机名） 可以 ping 通

       ```lang-javascript
       # ping server1
       ```
    2. ping server2（远端主机名） 可以 ping 通

       ```lang-javascript
       # ping server2
       ```

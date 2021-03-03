>**Note:**  
>集群环境需要在每台数据库服务器上执行如下操作：

- 以 root 身份登陆数据库服务器

- 执行如下命令卸载 SequoiaDB 软件

  ```lang-javascript
  $ /opt/sequoiadb/uninstall
  ```

- 回退系统配置参数  
  1. 删除配置文件 /etc/security/limits.conf 中的如下配置参数：  
    
     ```
     ?  <#domain>     <type>    <item>     <value>  
     ?  *               soft        core         0
     ?  *               soft        data         unlimited
     ?  *               soft        fsize        unlimited
     ?  *               soft        rss          unlimited
     ?  *               soft        as           unlimited
     ```
  2. 删除配置文件 /etc/sysctl.conf 中的如下配置参数：

     ```
     vm.swappiness = 0
     vm.dirty_ratio = 100
     vm.dirty_background_ratio = 10
     vm.dirty_expire_centisecs = 50000
     vm.vfs_cache_pressure = 200
     vm.min_free_kbytes = <物理内存大小的8%，单位KB>
     ```

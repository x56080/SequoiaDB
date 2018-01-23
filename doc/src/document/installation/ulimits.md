如果您采用Linux操作系统，在安装 SequoiaDB 产品之后，建议您根据实际情况配置数据库进程的ulimit，以保障系统的稳定高效运行。

##调整 ulimit##


 配置安装目录下的 conf/limits.conf 文件，修改配置后需[重启服务](database_management/sdbcm.md)：  

 ```
 core_file_size=0
 data_seg_size=-1
 file_size=-1
 virtual_memory=-1
 open_files=60000
 ```

 **参数说明：**  
 **core_file_size**：数据库出现故障时产生 core 文件用于故障诊断，生产系统建议关闭，默认是0；

 **data_seg_size**：数据库进程所允许分配的数据段大小，默认是-1；

 **file_size**：数据库进程所允许寻址的文件大小，默认是-1；

 **virtual_memory**：数据库进程所允许最大虚拟内存寻址空间限制，默认是-1；

 **open_files**：数据库进程允许的最大文件句柄数，默认是60000；

>Note:
>
>-1表示unlimited。

如果采用Linux操作系统，在安装 SequoiaDB 产品之后，建议根据实际情况配置数据库进程的 ulimit，以保障系统的稳定高效运行。

##调整 ulimit##


配置安装目录下的 conf/limits.conf 文件，修改后需[重启服务](database_management/sdbcm.md)才能生效。  

```lang-ini
core_file_size=0
data_seg_size=-1
file_size=-1
virtual_memory=-1
open_files=60000
stack_size=524288
```
 
 * 配置描述：

   | 配置项         | 描述                                                       | 默认值              | 单位 |
   | -------------- | ---------------------------------------------------------- | ------------------- | ---- |
   | core_file_size | 出现故障时产生 core 文件，用于故障诊断，生产系统建议关闭。 | 0                   | 字节 |
   | data_seg_size  | 进程所允许分配的数据段大小。                               | -1（表示unlimited） | 字节 |
   | file_size      | 进程所允许寻址的文件大小。                                 | -1（表示unlimited） | 字节 |
   | virtual_memory | 进程所允许最大虚拟内存寻址空间限制。                       | -1（表示unlimited） | 字节 |
   | open_files     | 进程允许的最大文件句柄数。                                 | 60000               | 文件个数 |
   | stack_size     | 进程允许的最大栈空间大小。                                 | 524288              | 字节 |


   >**Note:**
   >
   > * conf/limits.conf 只能配置以上参数，其他 ulimit 值，如最大进程数，由当前 Linux Shell 中的 ulimit 值决定。
   > * service sdbcm sdbcm、bin/sdbcmart、bin/sdbstart 启动节点和 sdbcm 时，均会引用该配置文件中的配置。
   > * 如果不想使用 conf/limits.conf 中的配置，可使用 -i 参数：bin/sdbcmart -i ; bin/sdbstart -i 。
   > * 创建节点时，新节点的 ulimit 值跟随 sdbcm 进程的 ulimit 值 。

 * 常见错误：

   非 root 用户设置 ulimit 时，一般情况下，不允许突破 hard limit 的限制。

   如果 conf/limits.conf 配置的值大于当前用户的 hard limit，则会报错。这时需提高 hard limit，或者使用 -i 参数。

   ```lang-bash
   $ ./bin/sdbstart -p 11810
   Error: Failed to set ulimit[open files] to [60000]
   Error: start sequoiadb will set ulimit by file[conf/limits.conf], if you want to set ulimit by current terminal, please use parameter '-i'.
   $ 
   $ # hard limit 值为1024，大于设置的60000
   $ ulimit -Hn
   1024
   ```

##查看ulimit##

查看数据库进程的ulimit

```lang-bash
$ cd /opt/sequoiadb
$
$ ./bin/sdblist
sequoiadb(11800) (20111) C
sequoiadb(11830) (20129) D
sequoiadb(11810) (20132) S
$
$ # 查看协调节点进程20132的ulimit信息
$ cat /proc/20132/limits 
Limit                     Soft Limit           Hard Limit           Units     
Max cpu time              unlimited            unlimited            seconds   
Max file size             unlimited            unlimited            bytes     
Max data size             unlimited            unlimited            bytes     
Max stack size            524288               524288               bytes     
Max core file size        0                    0                    bytes     
Max resident set          unlimited            unlimited            bytes     
Max processes             23711                23711                processes 
Max open files            60000                60000                files     
Max locked memory         65536                65536                bytes     
Max address space         unlimited            unlimited            bytes     
Max file locks            unlimited            unlimited            locks     
Max pending signals       23711                23711                signals   
Max msgqueue size         819200               819200               bytes     
Max nice priority         0                    0                    
Max realtime priority     0                    0                    
Max realtime timeout      unlimited            unlimited            us     
```

也可通过[节点健康检测快照](manual/Maintainance/Monitoring/snapshot/SDB_SNAP_HEALTH.md)查询各个节点的ulimit信息

```lang-javascript
> db=new Sdb( 'localhost: 11810' )
> db.snapshot( SDB_SNAP_HEALTH )
{
  ...
  "Ulimit": {
    "CoreFileSize": -1,
    "VirtualMemory": -1,
    "OpenFiles": 1024,
    "NumProc": 23948,
    "FileSize": -1,
    "StackSize": 524288
  }
  ...
}
...
```


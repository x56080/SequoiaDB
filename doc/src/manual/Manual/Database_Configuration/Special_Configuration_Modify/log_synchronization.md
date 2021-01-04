##日志文件##

SequoiaDB 采用日志方式进行副本间的数据同步。日志文件存在于 replicalog 目录中。文件大小和个数可以分别通过 logfilesz 和 logfilenum 参数进行设置。默认分别为 64MB（不包含头大小）和 20。参数生效后无法修改。（如果要修改必须离线删除全部日志文件，重新配置参数并启动 SequoiaDB。但此举通常会引起全量同步。）

##同步##

数据组内所有备节点会定期将其他数据节点日志打包下载到本地进行日志回放。同步源并不限于主节点。因为我们期望所有节点的数据版本差距在一个很小的窗口内。当处于这个窗口内时，所有备节点向主节点同步数据。但是当某些节点的数据版本与主节点相差过大时，则选择其他备节点进行同步。当发生版本冲突时，以当前主节点数据版本为准。如果冲突不能解决则进入全量同步。当组内不存在主节点时，同步无法进行。

##全量同步##

触发全量同步的原因有：

1.  宕机重启。
2.  节点数据版本与其他节点相差过大。
3.  数据不一致并且无法修复。

>   **Note:**
>
>   正常重启后，如果数据版本仍在可同步范围内则不会触发全量同步。

发生全量同步的节点会清空本地所有数据及日志，同时将组内另一个节点（不限于主节点）的数据全部复制到本地。期间同步源发生的数据改变同样会被复制到本地。全量同步期间本节点对外不提供服务。当组内不存在主节点时，全量同步无法进行。全量同步会极大地影响整个组的性能，甚至导致其他备节点同步性能降低。建议通过增加分区及日志容量来避免全量同步。

##配置同步日志参数##
1. 关闭要修改配置的节点11820。

  ```lang-javascript
  $ sdbstop -p 11820
  ```

2. 进入该节点目录，删除replicalog 目录。

  ```lang-bash
  $ cd /opt/sequoiadb/database/data/11820
  $ rm -rf replicalog/
  ```

3. 进入该节点的配置文件所在位置，重新配置参数。将 logfilesz 设为70， logfilenum 设为30。如果没有 logfilesz 和 logfilenum ，请添加 logfilesz =70， logfilenum =30这两行。

  ```lang-bash
  $ cd /opt/sequoiadb/conf/local/11820
  $ vim sdb.conf
  ```
  配置文件内容如下：

  ```lang-ini
  ...
  logfilesz=70
  logfilenum=30
  ...
  ```

4. 重新启动节点。

  ```lang-javascript
  $ sdbstart -p 11820
  ```

5. 连接协调节点11810，使用快照查看节点11820的配置参数。

  ```lang-javascript
  > var db=new Sdb("localhost",11810)
  > db.snapshot(SDB_SNAP_CONFIGS,{"svcname":"11820"},{"logfilesz":"","logfilenum":""})
  {
    "logfilesz": 70,
    "logfilenum": 30
  }
  ```
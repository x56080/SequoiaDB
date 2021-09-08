##dbpath修改##

indexpath、lobpath、lobmetapath 默认与 dbpath 路径相同。在没有指定它们路径的情况下改变 dbpath，不仅要将数据文件进行转移，还要将索引文件、大对象数据文件、大对象元数据文件一并转移。如果指定了它们的路径，则不需要转移。<br>以下均为默认存储路径，将 dbpath 由：/opt/sequoiadb/database/data/11820，修改为：/opt/sequoiadb/database/data/11820/dbpath。

1. 关闭要修改配置的节点11820。

  ```lang-javascript
  $ sdbstop -p 11820
  ```

2. 进入该节点数据文件所在位置，创建新的数据文件存储目录 dbpath。设置目录权限，将原有的数据文件 *.data、索引文件 *.idx、大对象数据文件 *.lobd、大对象元数据文件 *.lobm转移到新的目录。

  ```lang-bash
  $ cd /opt/sequoiadb/database/data/11820
  $ mkdir dbpath
  $ chown -R sdbadmin:sdbadmin_group dbpath/
  $ chmod 755 dbpath/
  $ mv *.data *.idx *.lobd *.lobm dbpath/
  ```

 >   **Note:**
 >
 >   系统默认情况下没有大对象文件。<br>
 >   注意新创建目录的权限问题。其中 sdbadmin:sdbadmin_group 为 sequoiadb 安装的用户名和用户组。

3. 进入该节点的配置文件所在位置，重新配置参数。将 dbpath 修改为 /opt/sequoiadb/database/data/11820/dbpath。

  ```lang-bash
  $ cd /opt/sequoiadb/conf/local/11820
  $ vim sdb.conf
  ```

  修改配置文件如下：

  ```lang-ini
  ...
  dbpath=/opt/sequoiadb/database/data/11820/dbpath
  ...
  ```

4. 重新启动节点。

  ```lang-javascript
  $ sdbstart -p 11820
  ```

5. 新创建的 dbpath 目录下会自动同步生成备份文件 bakfile/*、同步日志文件 replicalog/*、归档日志文件 archivelog/*。原来对应的文件和目录可以删除。
  
  ```lang-bash
  $ cd /opt/sequoiadb/database/data/11820
  $ rm -rf replicalog/ bakfile/ archivelog/
  ```
  
6. 连接协调节点11810，使用快照查看节点11820的配置参数。

  ```lang-javascript
  > var db=new Sdb("localhost",11810)
  > db.snapshot(SDB_SNAP_CONFIGS,{"svcname":"11820"},{"dbpath":""})
  {
  "dbpath": "/opt/sequoiadb/database/data/11820/dbpath/"
  }
  ```
##indexpath修改##

indexpath 默认与 dbpath 相同。以下将 indexpath 由：/opt/sequoiadb/database/data/11820，修改为：/opt/sequoiadb/database/data/11820/indexpath。

1. 关闭要修改配置的节点11820。

  ```lang-javascript
  $ sdbstop -p 11820
  ```

2. 进入该节点索引文件所在位置，创建新的索引文件存储目录 indexptah。将原有的索引文件 *.idx转移到新的目录。

  ```lang-bash
  $ cd /opt/sequoiadb/database/data/11820
  $ mkdir indexpath
  $ chown -R sdbadmin:sdbadmin_group indexpath/
  $ chmod 755 indexpath/
  $ mv *.idx indexpath/
  ```

 >   **Note:**
 >
 >   注意新创建目录的权限问题。其中 sdbadmin:sdbadmin_group 为 sequoiadb 安装的用户名和用户组。

3. 进入该节点的配置文件所在位置，重新配置参数。将 indexpath 修改为 /opt/sequoiadb/database/data/11820/indexpath。

  ```lang-bash
  $ cd /opt/sequoiadb/conf/local/11820
  $ vim sdb.conf
  ```

  修改配置文件如下：

  ```lang-ini
  ...
  indexpath=/opt/sequoiadb/database/data/11820/indexpath
  ...
  ```

4. 重新启动节点。

  ```lang-javascript
  $ sdbstart -p 11820
  ```

5. 连接协调节点11810，使用快照查看节点11820的配置参数。

  ```lang-javascript
  > var db=new Sdb("localhost",11810)
  > db.snapshot(SDB_SNAP_CONFIGS,{"svcname":"11820"},{"indexpath":""})
  {
  "indexpath": "/opt/sequoiadb/database/data/11820/indexpath/"
  }
  ```
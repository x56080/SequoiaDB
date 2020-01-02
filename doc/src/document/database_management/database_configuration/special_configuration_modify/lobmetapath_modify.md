##lobmetapath修改##

lobmetapath 默认与 lobpath 相同。以下将 lobmetapath 由：/opt/sequoiadb/database/data/11820，修改为：/opt/sequoiadb/database/data/11820/lobmetapath。

1. 关闭要修改配置的节点11820。

  ```lang-javascript
  $ sdbstop -p 11820
  ```

2. 进入该节点大对象元数据文件所在位置，创建新的大对象元数据目录 lobmetapath。将该原有的大对象元数据文件 *.lobm转移到新的目录。

  ```lang-bash
  $ cd /opt/sequoiadb/database/data/11820
  $ mkdir lobmetapath
  $ chown -R sdbadmin:sdbadmin_group lobmetapath/
  $ chmod 755 lobmetapath/
  $ mv *.lobm lobmetapath/
  ```

 >   **Note:**
 >    
 >   注意新创建目录的权限问题。其中 sdbadmin:sdbadmin_group 为 sequoiadb 安装的用户名和用户组。

3. 进入该节点的配置文件所在位置，重新配置参数。将 lobmetapath 修改为 /opt/sequoiadb/database/data/11820/lobmetapath。

  ```lang-bash
  $ cd /opt/sequoiadb/conf/local/11820
  $ vim sdb.conf
  ```

  修改配置文件如下：

  ```lang-ini
  ...
  lobmetapath=/opt/sequoiadb/database/data/11820/lobmetapath
  ...
  ```

4. 重新启动节点。

  ```lang-javascript
  $ sdbstart -p 11820
  ```

5. 连接协调节点11810，使用快照查看节点11820的配置参数。

  ```lang-javascript
  > var db=new Sdb("localhost",11810)
  > db.snapshot(SDB_SNAP_CONFIGS,{"svcname":"11820"},{"lobmetapath":""})
  {
  "lobmetapath": "/opt/sequoiadb/database/data/11820/lobmetapath/"
  }
  ```
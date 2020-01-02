##lobpath修改##

lobmetapath 默认与 lobpath 相同。在没有指定 lobmetapath 的情况下改变 lobpath，要将大对象数据文件和大对象元数据文件一并转移。如果指定了 lobmetapath 路径，则不需要转移。<br>以下配置均为默认存储路径，将 lobpath 由：/opt/sequoiadb/database/data/11820，修改为：/opt/sequoiadb/database/data/11820/lobpath。

1. 关闭要修改配置的节点11820。

  ```lang-javascript
  $ sdbstop -p 11820
  ```

2. 进入该节点大对象数据文件所在位置，创建新的大对象数据存储目录 lobpath。将原有的大对象数据文件 *.lobd和大对象元数据文件 *.lobm转移到新的目录。

  ```lang-bash
  $ cd /opt/sequoiadb/database/data/11820
  $ mkdir   lobpath
  $ chown -R sdbadmin:sdbadmin_group lobpath/
  $ chmod 755 lobpath/
  $ mv *.lobd *.lobm lobpath/
  ```

  >   **Note:**
  >
  >   注意新创建目录的权限问题。其中 sdbadmin:sdbadmin_group 为 sequoiadb 安装的用户名和用户组。

3. 进入该节点的配置文件所在位置，重新配置参数。将 lobpath 修改为 /opt/sequoiadb/database/data/11820/lobpath。

  ```lang-bash
  $ cd /opt/sequoiadb/conf/local/11820
  $ vim sdb.conf
  ```

  修改配置文件如下：

  ```lang-ini
  ...
    lobpath=/opt/sequoiadb/database/data/11820/lobpath
  ...
  ```

4. 重新启动节点。

  ```lang-javascript
  $ sdbstart -p 11820
  ```

5. 连接协调节点11810，使用快照查看节点11820的配置参数。

  ```lang-javascript
  > var db=new Sdb("localhost",11810)
  > db.snapshot(SDB_SNAP_CONFIGS,{"svcname":"11820"},{"lobpath":""})
  {
  "  lobpath": "/opt/sequoiadb/database/data/11820/lobpath/"
  }
  ```
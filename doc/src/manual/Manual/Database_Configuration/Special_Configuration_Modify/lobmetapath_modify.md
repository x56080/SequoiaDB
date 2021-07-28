lobmetapath 用于修改大对象元数据文件的存储路径，默认为 lobpath 指定的路径。修改前需要将原路径下的大对象元数据文件转移至目标路径，以节点 11820 为例，具体操作如下：

1. 停止节点 11820

    ```lang-bash
    $ sdbstop -p 11820
    ```

2. 创建新的大对象元数据文件存储目录，并修改目录权限为数据库管理用户（安装 SequoiaDB 时指定，默认为 sdbadmin）

    ```lang-bash
    $ mkdir /opt/sequoiadb/lobmetapath_11820
    $ chown -R sdbadmin:sdbadmin_group /opt/sequoiadb/lobmetapath_11820
    $ chmod 755 /opt/sequoiadb/lobmetapath_11820
    ```

3. 切换至原路径（默认为 `/opt/sequoiadb/database/data/11820`）

    ```lang-bash
    $ cd /opt/sequoiadb/database/data/11820
    ```

4. 将大对象元数据文件转移至目标路径

    ```lang-bash
    $ mv *.lobm /opt/sequoiadb/lobmetapath_11820
    ```

5. 修改节点 11820 的配置文件

    ```lang-bash
    $ vim /opt/sequoiadb/conf/local/11820/sdb.conf
    ```

    将参数 lobmetapath 修改为 `/opt/sequoiadb/lobmetapath_11820`

    ```lang-ini
    ...
    lobmetapath=/opt/sequoiadb/lobmetapath_11820
    ...
    ```

6. 重启节点 11820

    ```lang-bash
    $ sdbstart -p 11820
    ```

7. 通过快照查看节点 11820 的配置信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CONFIGS, {"svcname": "11820"}, {"lobmetapath": ""})
    ```
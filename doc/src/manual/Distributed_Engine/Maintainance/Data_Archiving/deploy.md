## 安装部署前准备

- 安装部署过程需使用 root 用户权限。

- 需参照《软硬件配置要求》调整系统配置。

- 提前搭建好数据源集群，并在主站点上创建数据源，并且配置数据源的 TransPropagateMode 为 notsupport。

   > **说明**
   >
   > - 集合数据源关联之后，默认情况下不允许在数据源上进行事务操作，对操作直接报错，因此需要将数据源的 TransPropagateMode 属性设置为 notsupport。 需要注意的是，notsupport 表示事务操作在数据源上不受支持，如果在事务中操作数据源，对应的操作会降级为非事务操作后发送到数据源处理，在数据源上执行的操作不受事务保护。

## 快速安装部署步骤

下述安装过程将使用名称为 `sdb-schedule-5.8.6-release.tar.gz` 的软件包为示例。软件包位于 `SequoiaDB 安装目录/tools/sdb-schedule` 目录下。

1. 将产品包解压缩至当前主机

    ```lang-bash
    $ tar -zxvf `sdb-schedule-5.8.6-release.tar.gz` -C /opt/data/
    ```

2. 指定所属用户及用户组为 SequoiaDB 的安装用户、用户组，以 sdbadmin:sdbadmin_group 为例

    ```lang-bash
    $ chown -R sdbadmin:sdbadmin_group /opt/data/sdb-schedule
    ```

3. 切换为安装用户

    ```lang-bash
    $ su sdbadmin
    ```

4. 切换至快速部署工具目录

    ```lang-bash
    $ cd /opt/data/sdb-schedule/script
    ```
   
5. 编辑配置文件 `quickDeploy.conf`，配置 `rootsite` 连接信息、要部署的节点端口等信息

   ```shell
   $ cat quickDeploy.conf

   # rootsite info
   # rootsite sequoiadb coord urls
   rootsite_url="192.168.17.183:11810,192.168.31.25:11810"
   rootsite_user="sdbadmin"
   rootsite_password="sdbadmin1"
   
   # node ports
   node_list="9001,9002"
   
   # datasource names
   datasource_list="datasource1,datasource2"
   
   # system cs name
   system_cs_name="SDB_SCHEDULE_SYSTEM"
   # system cs store domain name
   system_cs_domain="domain1"
   ```

6. 执行快速部署脚本

    ```lang-bash
    $ bash quickDeploy.sh deploy
    ```
   
7. 若其他机器也需要部署调度服务节点，在其他机器上执行上述步骤 1-5，然后拷贝 `quickDeploy.conf` 文件内容至其他机器的 `quickDeploy.conf` 文件中，修改要部署的节点端口后，执行如下命令只创建调度节点即可

    ```lang-bash
    $ bash quickDeploy.sh createnode
    ```
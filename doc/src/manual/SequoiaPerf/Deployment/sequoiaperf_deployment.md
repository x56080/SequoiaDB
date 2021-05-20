[^_^]:
   部署Sequoiaperf实例（目录名称中，中英文不需要空行）

本文档主要介绍如何部署 SequoiaPerf 实例，及完成部署后 SequoiaPerf 的访问方法。

##检查服务状态##

在进行 SequoiaPerf 部署前，用户需要在每台服务器上检查 SequoiaPerf 配置服务的状态。

```lang-text
# service sequoiaperf status
```

系统提示“running”表示服务正在运行，否则执行如下命令重新配置服务程序：

```lang-text
# service sequoiaperf start
```

##部署##

用户需要通过 [sdb_perf_ctl][tool] 工具部署 SequoiaPerf 实例

下述操作步骤假设 SequoiaPerf 程序安装在 `/opt/sequoiaperf` 目录下。

>**Note:**
>
> SequoiaPerf 服务进程全部以 SequoiaPerf 配置用户（默认为 sdbadmin）运行，需确保 SequoiaPerf 安装目录都赋予 sdnadmin 读写权限。

1. 切换至 sdbadmin 用户

   ```lang-bash
   # su - sdbadmin
   ```

2. 切换至 SequoiaPerf 安装目录

   ```lang-bash
   $ cd /opt/sequoiaperf
   ```

3. 创建名为“SequoiaPerf1”的 Sequoiaperf 实例，指定集群协调节点地址为 `127.0.0.1:11810`，Sequoiaperf 页面服务器 ip 为 192.168.31.20

   ```lang-bash
   $ bin/sdb_perf_ctl addinst SequoiaPerf1 -c 127.0.0.1:11810 -e 192.168.31.20
   ```

4. 启动实例

   ```lang-bash
   $ bin/sdb_perf_ctl start SequoiaPerf1
   ```

5. 查看实例状态

   ```lang-bash
   $ bin/sdb_perf_ctl status
   ```

   系统提示 up 表示实例部署完成，用户可通过 SequoiaPerf 功能进行监控

   ```lang-text
   INSTANCE_NAME   STATUS          GW_PORT    INSTANCE_DIRECTORY                            FAILED_SERVICES
   SequoiaPerf1    up              14000      /home/yang/Workspace/Monitor_Project/sequoiaperf2/instances/SequoiaPerf1 NONE
   Total: 1; Run: 1

   ```

##访问SequoiaPerf##

部署完成后，SequoiaPerf 会自动开启 14000端口的 Web 服务，用户可以通过浏览器使用 SequoiaPerf。假设安装 SequoiaPerf Web 服务的服务器 IP 为 192.168.30.20，则在浏览器进入 `http://192.168.30.20:14000`

>**Note:**
>
> 为了保证分析信息图形展示以及报警信息呈现的准确性，前端监控浏览器所在客户端需要与sequoiaperf部署集群的时钟尽量保持同步，误差不超过15秒。


[^_^]:
    本文使用的所有引用及链接
[tool]:manual/SequoiaPerf/Tools/sdb_perf_ctl.md

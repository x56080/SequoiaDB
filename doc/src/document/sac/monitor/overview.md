SAC 监控可以查看存储集群和主机的运行状态。

- 完成[添加主机](sac/deployment/host/add_host.md)和[创建存储集群](sac/deployment/distributed_storage/create_storage.md)后，可以在 SAC 监控该存储集群。

  点击左侧导航 **监控**，点击 **存储集群的名字**，进入监控总览页面。

  ![总览](sac/monitor/overview.png)

  页面左侧可以了解主机数量、磁盘数量以及 CPU、内存、磁盘的使用情况。

  页面右侧有版本，会话、域、分区组、节点、集合、记录和Lob数量的统计信息；图表则可以看到当前 insert、update、delete、read 的实时速率。

  如果当前存储集群有异常时，页面下方将显示出警告和错误信息。

- 点击页面上方的 **节点** 、 **资源** 或者 **主机** 可分别进入查看详细的监控页面。
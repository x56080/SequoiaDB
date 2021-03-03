SAC监控可以查看当前业务和主机的运行状态。

- 完成[添加主机](sac/deployment/add_host/add_host.md)及[添加业务](sac/deployment/add_module/install_module.md)后，即可使用SequoiaDB监控服务。

  1.点击左侧导航栏 **监控** 选项，选择需要查看的业务。

  2.选择业务后，进入监控总览页面。
  ![](sac/monitor/overview.jpg)

  页面左侧可以了解主机数量、磁盘数量以及CPU、内存、磁盘的使用情况。

  页面右侧可以了解业务版本，会话、域、分区组、节点、集合、记录及Lob的数量，图表则可以看到当前insert、update、delete、read的实时速率。

  如果当前业务有异常时，页面下方将显示出当前业务的警告以及错误信息。

- 点击页面上方的 **节点** 、 **资源** 或者 **主机** 可分别进入查看详细的监控页面。
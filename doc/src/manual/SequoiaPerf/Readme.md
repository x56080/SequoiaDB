SequoiaPerf 是 SequoiaDB 巨杉数据库的性能分析工具。该工具通过图形化界面，使用户可以更直观地对 SequoiaSQL 和 SequoiaDB 的查询语句进行性能分析和监控。同时，提供整体集群资源的使用情况和告警配置，以保证资源高利用率和高可靠性。

SequoiaPerf 通过 SequoiaPerf 实例连接 SequoiaDB 集群，读取相关监控数据。每个 SequoiaPerf 实例包含以下服务：

- sequoiaperf_gateway：反向代理服务器，用于接收客户端请求并将其转发至 sequoiaperf 服务
- sequoiaperf_grafana：通用仪表板和图形编写器，可作为 SequoiaPerf 页面应用程序运行
- sequoiaperf_prometheus：在部署了时间序列节点的数据库中记录实时指标，用于定期从已配置的目标中提取指标，并将其存储在该数据库中
- sequoiaperf_exporter: SDB 信息导出器，用于连接 SequoiaDB 集群获取集群信息和慢查询信息。
- sequoiaperf_sql_exporter: SQL 慢查询信息导出器，用于连接目标 SQL 获取慢查询信息。
- sequoiaperf_mysqld_exporter: SQL 系统信息导出器，用于连接目标 SQL 获取SQL系统信息。
- sequoiaperf_alertmanger: 告警消息管理器，经过[配置][alertmanager_configuration]后可用于向目标下游如邮箱，微信发送即时告警信息。

本章主要内容如下：

+ [运行环境][environment]：准备 SequoiaPerf 所需的运行环境
+ [安装部署][installation]：安装 SequoiaPerf 和 部署 SequoiaPerf 实例
+ [概览][overview]：SequoiaPerf 所包含的概览界面介绍
+ [配置][configuration]：SequoiaPerf 配置页面，通过该界面可以对 SequoiaPerf 实例、SequoiaDB 集群、SQL 实例和告警进行配置
+ [慢查询监控][slowquery]：通过 SequoiaPerf 对 SequoiaDB 和 SQL 的慢查询进行监控
+ [工具][tools]：用于管理 SequoiaPerf 实例及对实例进行归档
+ [告警消息接收][alertmanager]：对 SequoiaPerf 告警消息接收方式进行配置
+ [卸载][uninstallation]：卸载 SequoiaPerf


[^_^]:
    本文使用的所有引用及链接
[environment]:manual/SequoiaPerf/sequoiaperf_operating_environment.md
[installation]:manual/SequoiaPerf/Deployment/Readme.md
[overview]:manual/SequoiaPerf/Overview/Readme.md
[configuration]:manual/SequoiaPerf/Configuration/Readme.md
[slowquery]:manual/SequoiaPerf/Slowquery_Monitor/Readme.md
[tools]:manual/SequoiaPerf/Tools/Readme.md
[alertmanager]:manual/SequoiaPerf/sequoiaperf_alertmanager_configuration.md
[uninstallation]:manual/SequoiaPerf/sequoiaperf_uninstallation.md

SdbDC 类主要用于操作集群，包含的函数如下：

| 名称 | 描述 |
|------|------|
| [activate()][activate] | 激活集群 |
| [deactivate()][deactivate] | 停用集群，集群不提供读写服务 |
| [disableReadonly()][disableReadonly] | 停止只读模式 |
| [enableReadonly()][enableReadonly] | 启用只读模式 |
| [getDetail()][getDetail] | 获取详细信息 |
| [setActiveLocation()][setActiveLocation] | 设置主位置集 |
| [setLocation()][setLocation] | 设置节点位置信息 |
| [startCriticalMode()][startCriticalMode] | 开启 Critial 模式 |
| [startMaintenanceMode()][startMaintenanceMode] | 开启运维模式 |
| [stopCriticalMode()][stopCriticalMode] | 停止 Critical 模式 |
| [stopMaintenanceMode()][stopMaintenanceMode] | 停止维护模式 |
| [reelect()][reelect] | 对符合条件的复制组重新选主 |
| [primarySave()][primarySave] | 保存复制组主节点信息 |
| [primaryRestore()][primaryRestore] | 根据保存的信息恢复复制组主节点 |
| [reelectAnalyse()][reelectAnalyse] | 分析主节点分布合理性或执行切主 |
| [locationAnalyse()][locationAnalyse] | 分析 Location 分布合理性 |

[^_^]:
     本文使用的所有引用及链接
[activate]:manual/Manual/Sequoiadb_Command/SdbDC/activate.md
[deactivate]:manual/Manual/Sequoiadb_Command/SdbDC/deactivate.md
[disableReadonly]:manual/Manual/Sequoiadb_Command/SdbDC/disableReadonly.md
[enableReadonly]:manual/Manual/Sequoiadb_Command/SdbDC/enableReadonly.md
[getDetail]:manual/Manual/Sequoiadb_Command/SdbDC/getDetail.md
[setActiveLocation]:manual/Manual/Sequoiadb_Command/SdbDC/setActiveLocation.md
[setLocation]:manual/Manual/Sequoiadb_Command/SdbDC/setLocation.md
[startCriticalMode]:manual/Manual/Sequoiadb_Command/SdbDC/startCriticalMode.md
[startMaintenanceMode]:manual/Manual/Sequoiadb_Command/SdbDC/startMaintenanceMode.md
[stopCriticalMode]:manual/Manual/Sequoiadb_Command/SdbDC/stopCriticalMode.md
[stopMaintenanceMode]:manual/Manual/Sequoiadb_Command/SdbDC/stopMaintenanceMode.md
[reelect]:manual/Manual/Sequoiadb_Command/SdbDC/reelect.md
[primarySave]:manual/Manual/Sequoiadb_Command/SdbDC/primarySave.md
[primaryRestore]:manual/Manual/Sequoiadb_Command/SdbDC/primaryRestore.md
[reelectAnalyse]:manual/Manual/Sequoiadb_Command/SdbDC/reelectAnalyse.md
[locationAnalyse]:manual/Manual/Sequoiadb_Command/SdbDC/locationAnalyse.md
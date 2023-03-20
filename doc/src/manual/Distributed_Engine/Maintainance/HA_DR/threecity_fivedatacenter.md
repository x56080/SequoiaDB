[^_^]:
    三地五中心部署

本文档主要介绍在三地五中心的部署方案下，如何应对不同级别的灾难。

##灾难应对方案##

###单节点故障###

由于采用了五副本高可用架构，个别节点故障情况下，集群依然可以正常工作。针对个别节点的故障场景，无需采取特别的应对措施，只需要及时修复故障节点，并通过自动数据同步或者人工数据同步的方式去恢复故障节点数据即可。

###单个数据中心整体故障###

当五个中心的一个发生故障时，每个数据组存活节点的数量还大于每个数据组的总节点数的 1/2，所以每个数据组仍然能够为应用层提供读写服务。针对单个数据中心整体故障的场景，无需采取特别的应对措施，只需要及时修复故障节点，并通过自动数据同步或者人工数据同步的方式去恢复故障节点数据即可。

###城市级整体故障###

当一个城市的所有数据中心发生故障时，每个数据组存活节点的数量还大于每个数据组的总节点数的 1/2，所以每个数据组仍然能够为应用层提供读写服务。针对单个城市整体故障的场景，无需采取特别的应对措施，只需要及时修复故障节点，并通过自动数据同步或者人工数据同步的方式去恢复故障节点数据即可。

当两个城市的数据中心全部故障时，每个数据组存活节点数小于总数的 1/2，无法选出主节点。这种情况下就需要用到分裂（split）和合并（merge）工具来进行处理，把存活的节点分裂成独立的集群提供读写服务。分裂集群的耗时相对比较短，一般在十分钟内便能完成。

##灾难恢复##

用户可以参考[容灾工具的使用][threecity_fivedatacenter_usage]来解决故障。


[^_^]:
    本文使用到的所有链接

[threecity_fivedatacenter]:images/Distributed_Engine/Maintainance/HA_DR/threecity_fivedatacenter.png
[split_merge]:manual/database_management/tools/split_merge.md
[consistency]:manual/Distributed_Engine/Architecture/Replication/primary_secondary_consistency.md
[threecity_fivedatacenter_usage]:manual/Distributed_Engine/Maintainance/HA_DR/disaster_recovery_tool.md
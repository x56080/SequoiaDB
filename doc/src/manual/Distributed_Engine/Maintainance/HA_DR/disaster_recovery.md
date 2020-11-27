[^_^]:
    容灾
    作者：杨磊
    时间：20190521
    评审意见
    王涛：
    许建辉：
    市场部：20190604


容灾是指建立多地的容灾中心，该中心是主数据中心的一个可用复制，在灾难发生之后确保原有的数据不会丢失或遭到破坏。容灾中心的数据可以是主中心生产数据的完全实时复制，也可以比主中心数据稍微落后，但一定是可用的。SequoiaDB 巨杉数据库的容灾机制主要采用数据复制与备份恢复技术。

大多数数据中心还需要“双活”的容灾能力，即两个数据中心数据库同时在线运行，处于可读可查询状态。“多活”一方面是多中心之间地位均等，正常模式下协同工作，并行的为业务访问提供服务，实现了对资源的充分利用，避免一个或两个备份中心处于闲置状态，造成资源与投资浪费；另一方面是在一个数据中心发生故障或灾难的情况下，其他数据中心可以正常运行并对关键业务或全部业务实现接管，实现用户的“故障无感知”。

SequoiaDB 巨杉数据库已经在内部实现了容灾备份以及“双活”的机制，主要特点包括：

- 异地容灾：异地的容灾和备份，保证数据安全，中心间距离超过 1000km 以上
- 同城容灾：同城双中心数据强一致实时同步，保证极端情况下数据不错不丢，RPO=0，RTO 小于十分钟
- 同城双活：同城双中心的数据强一致实时同步，保证数据一致；双中心数据可以实现同时读写，大大提升读写效率；中心切换 RTO 小于十分钟，RPO=0
- 更便捷的灾备管理：系统集群中统一管理灾备中心，简化维护成本，帮助用户更快上手


SequoiaDB 提供的容灾方案有：[同城双中心][twodatacenter]、[同城三中心][threedatacenter]、[两地三中心][twocity_threedatacenter]和[三地五中心][threecity_fivedatacenter]。


[^_^]:
    本文使用到的所有链接

[twodatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/twodatacenter.md
[threedatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/threedatacenter.md
[twocity_threedatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/twocity_threedatacenter.md
[threecity_fivedatacenter]:manual/Distributed_Engine/Maintainance/HA_DR/threecity_fivedatacenter.md
当查询到达 SequoiaDB 时，需要经过协调节点和数据节点来共同完成查询：

1.  协调节点
    1.  根据匹配符筛选查询所在集合的分区键，并确定需要下发查询的数据组
    2.  如果带有排序，协调节点需要负责保证多个数据节点返回的数据按照排序字段进行排序

2.  数据节点
    1.  根据匹配符、排序字段筛选可用的索引，或者进行全表扫描，通过基于代价的估算最终选取适合的访问计划
    2.  如果有垂直分区并且带有排序，数据节点需要负责保证多个子表返回的数据按照排序字段进行排序
    3.  数据节点上可以缓存查询计划加快查询的过程

##相关内容##

1.  [基于代价的访问计划评估](database_management/access_plans/search_paths/overview.md)：查询计划优化器使用基于代价的方式对候选访问计划进行评估
2.  [统计信息](database_management/statistics/statistics.md)：统计信息可以帮助查询优化器获得更好的访问计划
3.  [查看访问计划](database_management/access_plans/detailed_access_plan/overview.md)：SdbQuery.explain() 的 Detail 选项可以查看详细的访问计划
4.  [访问计划缓存](database_management/access_plans/plan_cache.md)：数据节点上可以通过缓存访问计划来加速查询

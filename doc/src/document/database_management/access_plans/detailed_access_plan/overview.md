使用 [SdbQuery.explain\(\)](reference/Sequoiadb_command/SdbQuery/explain.md) 可以查看查询的访问计划。

当 SdbQuery.explain() 的 Detail 选项为 true 时，将会展示详细的访问计划。在协调节点和数据节点上展示的详细访问计划略有不同。

**协调节点上的详细访问计划**

协调节点上的详细访问计划包括以下内容：

1.  协调节点上的访问计划信息
2.  涉及数据节点上的访问计划信息

```lang-json
{
  { 协调节点的访问计划信息 },
  "PlanPath": {
    "Operator": "COORD-MERGE",
    { 协调节点查询上下文的访问计划信息 },
    "ChildOperators": [
      {
        { 数据节点的访问计划信息 },
        ...
      },
      ...
    ]
  }
}
```

详细请参考：[协调节点的访问计划](database_management/access_plans/detailed_access_plan/coord_plan.md)

**数据节点上的详细访问计划**

数据节点上的详细访问计划包括以下内容：

1.  数据节点上的访问计划信息
2.  访问计划的缓存使用情况
3.  涉及的垂直分区中主子表的访问计划信息

主表的详细访问计划：

```lang-json
{
  { 主表的访问计划信息 },
  "PlanPath": {
    "Operator": "MERGE",
    { 主表查询上下文的访问计划信息 },
    "ChildOperators": [
      {
        { 子表的访问计划信息 },
        ...
      },
      ...
    ]
  }
}
```

详细请参考：[主表的访问计划](database_management/access_plans/detailed_access_plan/main_collection_plan.md)

普通集合或者子表的详细访问计划：

```lang-json
{
  { 集合的访问计划信息 },
  "PlanPath": {
    { 查询上下文的访问计划信息 }
    ...
  }
}
```

详细请参考：[数据节点的访问计划](database_management/access_plans/detailed_access_plan/collection_plan.md)

详细的访问计划中有以下操作：

1.  [COORD-MERGE](database_management/access_plans/detailed_access_plan/COORD_MERGE.md)：对应一个协调节点上的查询上下文对象。
2.  [MERGE](database_management/access_plans/detailed_access_plan/MERGE.md)：对应一个数据节点上的主表查询上下文对象。
3.  [SORT](database_management/access_plans/detailed_access_plan/SORT.md)：对应一个数据节点上的排序上下文对象。
4.  [TBSCAN](database_management/access_plans/detailed_access_plan/TBSCAN.md)：对应一个数据节点上的全表扫描上下文对象。
5.  [IXSCAN](database_management/access_plans/detailed_access_plan/IXSCAN.md)：对应一个数据节点上的索引扫描上下文对象。

协调节点上的访问计划包括以下信息：

| 字段名      | 类型      | 描述                                    |
| ----------- | --------- | --------------------------------------- |
| NodeName    | 字符串    | 访问计划所在的节点的名称                |
| GroupName   | 字符串    | 访问计划所在的节点属于的复制组的名称    |
| Role        | 字符串    | 访问计划所在的节点的角色<br>"coord" 表示协调节点 |
| Collection  | 字符串    | 访问计划访问的集合的名称                |
| Query       | BSON 对象 | 访问计划解析后的用户查询条件            |
| Sort        | BSON 对象 | 访问计划中的排序字段                    |
| Selector    | BSON 对象 | 访问计划执行的选择符                    |
| Hint        | BSON 对象 | 访问计划中指定查询使用索引的情况        |
| Skip        | 长整型    | 访问计划需要跳过的记录个数              |
| Return      | 长整型    | 访问计划最多返回的记录个数              |
| Flag        | 整型      | 访问计划中指定的执行标志，默认值为 0    |
| ReturnNum   | 长整型    | 访问计划返回记录的个数                  |
| ElapsedTime | 浮点型    | 访问计划查询耗时（单位：秒）            |
| UserCPU     | 浮点型    | 访问计划用户态 CPU 使用时间（单位：秒） |
| SysCPU      | 浮点型    | 访问计划内核态 CPU 使用时间（单位：秒） |
| PlanPath    | BSON 对象 | 访问计划的具体执行操作 [COORD-MERGE](database_management/access_plans/detailed_access_plan/COORD_MERGE.md) |

>   **Note:**
>   [COORD-MERGE](database_management/access_plans/detailed_access_plan/COORD_MERGE.md) 中可能包含 [主表的访问计划](database_management/access_plans/detailed_access_plan/main_collection_plan.md) 或者 [数据节点的访问计划](database_management/access_plans/detailed_access_plan/collection_plan.md)

**示例**

```lang-json
{
  "NodeName": "hostname:11810",
  "GroupName": "SYSCoord",
  "Role": "coord",
  "Collection": "foo.bar",
  "Query": {
    "a": 1
  },
  "Sort": {},
  "Selector": {},
  "Hint": {
    "": ""
  },
  "Skip": 0,
  "Return": -1,
  "Flag": 0,
  "ReturnNum": 10,
  "ElapsedTime": 0.050839,
  "UserCPU": 0,
  "SysCPU": 0,
  "PlanPath": {
    "Operator": "COORD-MERGE",
    ...
  }
}
```

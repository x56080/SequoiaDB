数据节点上的访问计划（包括垂直分区中的子表）包括以下信息：

| 字段名                    | 类型      | 描述                                    |
| ------------------------- | --------- | --------------------------------------- |
| NodeName                  | 字符串    | 访问计划所在的节点的名称                |
| GroupName                 | 字符串    | 访问计划所在的节点属于的复制组的名称    |
| Role                      | 字符串    | 访问计划所在的节点的角色<br>"data" 表示数据节点 |
| Collection                | 字符串    | 访问计划访问的集合的名称                |
| Query                     | BSON 对象 | 访问计划解析后的用户查询条件            |
| Sort                      | BSON 对象 | 访问计划中的排序字段                    |
| Selector                  | BSON 对象 | 访问计划执行的选择符                    |
| Hint                      | BSON 对象 | 访问计划中指定查询使用索引的情况        |
| Skip                      | 长整型    | 访问计划需要跳过的记录个数              |
| Return                    | 长整型    | 访问计划最多返回的记录个数              |
| Flag                      | 整型      | 访问计划中指定的执行标志，默认值为 0    |
| ReturnNum                 | 长整型    | 访问计划返回记录的个数                  |
| ElapsedTime               | 浮点型    | 访问计划查询耗时（单位：秒）            |
| IndexRead                 | 长整型    | 访问计划扫描索引记录的个数              |
| DataRead                  | 长整型    | 访问计划扫描数据记录的个数              |
| UserCPU                   | 浮点型    | 访问计划用户态 CPU 使用时间（单位：秒） |
| SysCPU                    | 浮点型    | 访问计划内核态 CPU 使用时间（单位：秒） |
| CacheStatus               | 字符串    | 访问计划的缓存状态：<br>1. "NoCache" 为没有加入缓存<br>2. "NewCache" 为新建的缓存<br>3. "HitCache" 为命中的缓存 |
| MainCLPlan                | 布尔型    | 访问计划是否主表共享的查询计划          |
| CacheLevel                | 字符串    | 访问计划的缓存级别：<br>1. "OPT_PLAN_NOCACHE" 为不进行缓存<br>2. "OPT_PLAN_ORIGINAL" 为缓存原查询计划<br>3. "OPT_PLAN_NORMALZIED" 为缓存泛化后的查询计划<br>4. "OPT_PLAN_PARAMETERIZED" 为缓存参数化的查询计划<br>"OPT_PLAN_FUZZYOPTR" 为缓存参数化并带操作符模糊匹配的查询计划 |
| Parameters                | 数组类型  | 参数化的访问计划使用的参数列表          |
| MatchConfig               | BSON 对象 | 访问计划中的匹配符的配置                |
| MatchConfig.EnableMixCmp  | 布尔型    | 访问计划的匹配符是否使用混合匹配模式    |
| MatchConfig.Parameterized | 布尔型    | 访问计划的匹配符是否支持参数化          |
| MatchConfig.FuzzyOptr     | 布尔型    | 访问计划的匹配符是否支持模糊匹配        |
| PlanPath                  | BSON 对象 | 访问计划的具体执行操作 [SORT](database_management/access_plans/detailed_access_plan/SORT.md)、[TBSCAN](database_management/access_plans/detailed_access_plan/TBSCAN.md) 或 [IXSCAN](database_management/access_plans/detailed_access_plan/IXSCAN.md) |
| Search                    | BSON 对象 | 查询计划优化器搜索过的访问计划<br>Search 选项为 true 时显示<br>请参考 [基于代价的访问计划评估](database_management/access_plans/search_paths/overview.md) |

>   **Note:**
>   数据节点上的主表的访问计划请参考 [主表的访问计划](database_management/access_plans/detailed_access_plan/main_collection_plan.md)

**示例**

```lang-json
{
  "NodeName": "hostname:11820",
  "GroupName": "group",
  "Role": "data",
  "Collection": "foo.bar",
  "Query": {
    "a": {
      "$gt": 100
    }
  },
  "Sort": {},
  "Selector": {},
  "Hint": {},
  "Skip": 0,
  "Return": -1,
  "Flag": 2048,
  "ReturnNum": 0,
  "ElapsedTime": 0.000093,
  "IndexRead": 0,
  "DataRead": 0,
  "UserCPU": 0,
  "SysCPU": 0,
  "CacheStatus": "HitCache",
  "MainCLPlan": false,
  "CacheLevel": "OPT_PLAN_PARAMETERIZED",
  "Parameters": [
    100
  ],
  "MatchConfig": {
    "EnableMixCmp": false,
    "Parameterized": true,
    "FuzzyOptr": false
  },
  "PlanPath": {
    ...
  }
```

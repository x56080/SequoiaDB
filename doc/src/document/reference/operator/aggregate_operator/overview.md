##聚集符##

| 参数名 | 描述 | 示例 |
| ------ | ---- | ---- |
| [$project](reference/operator/aggregate_operator/project.md) | 选择需要输出的字段名，"1"表示输出，"0"表示不输出，还可以实现字段的重命名 |{ $project: { field1: 1, field: 0, aliase: "$field3" } } |
| [$match](reference/operator/aggregate_operator/match.md) | 实现从集合中选择匹配条件的记录，相当于SQL语句的where | {$match: { field: { $lte: value } } } |
| [$limit](reference/operator/aggregate_operator/limit.md) | 限制返回的记录条数 | { $limit: 10 } |
| [$skip](reference/operator/aggregate_operator/skip.md) | 控制结果集的开始点，即跳过结果集中指定条数的记录 | { $skip: 5 } |
| [$group](reference/operator/aggregate_operator/group.md) | 实现对记录的分组，类似于SQL的group by语句，"_id"指定分组字段 | { $group: { _id: "$field" } } |
| [$sort](reference/operator/aggregate_operator/sort.md) | 实现对结果集的排序，"1"代表升序，"-1"代表降序 | { $sort: { field1: 1, field2: -1, ... } } |

>  **Note:**
>
>  请参考 [SdbCollection.aggregate\(\)](reference/Sequoiadb_command/SdbCollection/aggregate.md)

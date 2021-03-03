聚集框架提供了对集合中的原始数据记录进行统计计算的能力。通过使用聚集框架，用户能够直接从集合中提取数据记录并获取所需的统计结果。聚集框架提供的操作接口类似于集合中的查询操作，不同的是聚集框架还提供了一系列[函数](reference/operator/aggregate_operator/overview.md)及操作对查询结果进行处理。

##aggregate()##

以下是聚集操作举例：

```lang-javascript
> db.foo.bar.aggregate( { $match: { age: { $gt: 30 } } }, { $group: { _id: "$city", income: { "$avg": "$income" }, city: { "$first": "$city" } } } )
```

上例聚集操作包含了两个子操作：

  -   ["$match"](reference/operator/aggregate_operator/match.md)子操作将集合中年龄大于30的数据记录筛选出来；
  -   ["$group"](reference/operator/aggregate_operator/group.md)子操作从筛选出的数据记录按照城市进行分组，计算出每个城市的人均收入。
  
通过上例聚集操作将得到各城市30岁以上的人均收入。

假设原始数据为：

```lang-json
{ name: "张三", income: 10000, age: 31, city: "北京" }
{ name: "李四", income: 25000, age: 28, city: "上海" }
{ name: "王五", income: 15000, age: 32, city: "上海" }
{ name: "赵六", income: 30000, age: 40, city: "北京" }
```

经过子操作$match后，数据为：

```lang-json
{ name: "张三", income: 10000, age: 31, city: "北京" }
{ name: "王五", income: 15000, age: 32, city: "上海" }
{ name: "赵六", income: 30000, age: 40, city: "北京" }
```

经过子操作$group后，最终结果为：

```lang-json
{ income: 15000, city: "上海" }
{ income: 20000, city: "北京" }
```

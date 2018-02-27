##语法##

```
{ $group: { _id："$分组字段名", 显示字段名: { 聚集函数: "$字段名"}，[显示字段名2: { 聚集函数: "$字段名"}, ...] } }
```

##说明##

$group实现对结果集的分组，类似SQL中的group by语句。

首先指定分组键（_id） ，通过“_id”来标识分组字段，分组字段可以是单个，也可以是多个，格式如下：

单个分组键：

```
{ _id: "$field" }
```

多个分组键：

```
{ _id: { field1: "$field1", field2: "$field2", ... } }
```

##示例##

$group使用如下：

```lang-javascript
> db.foo.bar.aggregate({ $group: { _id: "$major", avg_score: { $avg: "$score" }, Major: { $first: "$major" } } })
{
  "avg_score": 82,
  "Major": "光学"
}
{
  "avg_score": 77.25,
  "Major": "物理学"
}
```

该操作表示从集合foo.bar中读取记录，并按major字段进行分组。在返回的结果集中，取各分组的第一条记录的major字段，重命名为Major；对各分组中的score字段值求平均值，重命名为avg_score。返回如下所示：

##$group支持的聚集函数##

|  函数名   |                   描述                                             |
| --------- | ------------------------------------------------------------------ |
| $addtoset | 将字段添加到数组中，相同的字段值只会添加一次                       |
| $first    | 取分组中第一条记录中的字段值                                       |
| $last     | 取分组中最后一条记录中的字段值                                     |
| $max      | 取分组中字段值最大的                                               |
| $min      | 取分组中字段值最小的                                               |
| $avg      | 取分组中字段值的平均值                                             |
| $push     | 将所有字段添加到数组中，即使数组中已经存在相同的字段值，也继续添加 |
| $sum      | 取分组中字段值的总和                                               |
| $count    | 对记录分组后，返回表所有的记录条数                                 |


##$addtoset##

记录分组后，使用$addtoset 将指定字段值添加到数组中，相同的字段值只会添加一次。对嵌套对象使用点操作符（.）引用字段名。

###示例###

如下操作对记录分组后将指定字段值添加到数组中输出：

```lang-javascript
> db.foo.bar.aggregate({ $group: { _id: "$dep", Dep: { $first: "$dep" }, addtoset_major: { $addtoset: "$major" } } })
{
  "Dep": "物电学院",
  "addtoset_major": [
    "物理学",
    "光学",
    "电学"
  ]
}
{
  "Dep": "计算机学院",
  "addtoset_major": [
    "计算机科学与技术",
    "计算机软件与理论",
    "计算机工程"
  ]
}
```

此操作对记录按dep字段值进行分组，并使用$first输出每个组第一条记录的dep字段，输出字段名为Dep；又将 major字段的值使用$addtoset 放入数组中返回，输出字段名为addtoset_major，如下：

##$count##

记录分组后，用$count 取出分组所包含的总记录条数。

###示例###

对记录分组后，返回表所有的记录条数：

```lang-javascript
> db.foo.bar.aggregate({ $group: { Total: { $count: "$dep" } } })
{
  "Total": 1001
}
```

##$first##

记录分组后，取分组中第一条记录指定的字段值，对嵌套对象使用点操作符（.）引用字段名。

###示例###

对记录分组后，输出每个分组第一条记录的指定字段值：

```lang-javascript
> db.foo.bar.aggregate({ $group: { _id: "$dep", Dep: { $first: "$dep" }, Name: { $first: "$info.name" } } })
{
  "Dep": "物电学院",
  "Name": "Lily"
}
{
  "Dep": "计算机学院",
  "Name": "Tom"
}
```

此操作对记录按dep字段分组，取每个分组中第一条记录的dep字段值和嵌套对象name字段值，输出字段名分别为 Dep 和 Name。

##$avg##

记录分组后，取分组中指定字段的平均值返回，对嵌套对象使用点操作符（.）引用字段名。

###示例###

对记录分组后，返回分组中指定字段的平均值：

```lang-javascript
> db.foo.bar.aggregate({ $group: { _id: "$dep", avg_age: { $avg: "$info.age" }, max_age: { $max: "$info.age" }, min_age: { $min: "$info.age" } } })
{
  "avg_age": 23.727273,
  "max_age": 36,
  "min_age": 15
}
{
  "avg_age": 24.5,
  "max_age": 30,
  "min_age": 20
}
```

此操作对记录按dep字段分组，使用$avg返回每个分组中的嵌套对象age字段的平均值，输出字段名为avg_age；又使用$min返回每个分组中嵌套对象age字段的最小值，输出字段名为min_age，使用$max 返回每个分组中嵌套对象 age字段的最大值，输出字段名为max_age。

##$max##

记录分组后，取分组中指定字段的最大值返回，对嵌套对象使用点操作符（.）引用字段名。

###示例###

对记录分组后，返回分组中指定字段的最大值：

```lang-javascript
> db.foo.bar.aggregate({ $group: { _id: "$dep", max_score: { $max:"$score" }, Name: { $last: "$info.name" } } })
{
  "max_score": 93,
  "Name": "Kate"
}
{
  "max_score": 90,
  "Name": "Jim"
}
```

此操作对记录按dep字段分组，使用$max 返回每个分组中score 字段的最大值，输出字段名为max_score，又使用 $last取每个分组中最后一条记录嵌套对象name字段值，输出字段名为Name。

##$min##

记录分组后，取分组中指定字段的最小值返回，对嵌套对象使用点操作符（.）引用字段名。

###示例###

对记录分组后，返回分组中指定字段的最小值：

```lang-javascript
> db.foo.bar.aggregate({ $group: { _id: "$dep", min_score: { $min: "$score" }, Name: { $last: "$info.name" } } })
{
  "min_score": 72,
  "Name": "Kate"
}
{
  "min_score": 69,
  "Name": "Jim"
}
```

此操作对记录按dep字段分组，使用$min 返回每个分组中score字段的最小值，输出字段名为min_score，又使用 $last取每个分组中最后一条记录嵌套对象name字段值，输出字段名为Name。

##$last##

记录分组后，取分组中最后一条记录指定的字段值，对嵌套对象使用点操作符（.）引用字段名。

###示例###

对记录分组后，输出每个分组最后一条记录的指定字段值：

```lang-javascript
> db.foo.bar.aggregate({ $group: { _id: "$dep", Major: { $addtoset: "$major" }, Name: { $last: "$info.name" } } })
{
  "Major": [
    "物理学",
    "光学",
    "电学"
  ],
  "Name": "Kate"
}
{
  "Major": [
    "计算机科学与技术",
    "计算机软件与理论",
    "计算机工程"
  ],
  "Name": "Jim"
}
```

此操作对记录按dep字段分组，使用$last取每个分组中最后一条记录嵌套对象name字段值，输出字段名为Name，并且将每个分组中的major字段值使用$addtoset填充到数组中返回，返回字段名为Major。

##$push##

记录分组后，使用$push将指定字段值添加到数组中，即使数组中已经存在相同的值，也继续添加。对嵌套对象使用点操作符（.）引用字段名。

###示例###

如下操作对记录分组后将指定字段值添加到数组中输出：

```lang-javascript
> db.foo.bar.aggregate({ $group: { _id: "$dep", Dep: { $first: "$dep" }, push_age: { $push: "$info.age" } } })
{
  "Dep": "物电学院",
  "push_age": [
    28,
    18,
    20,
    30,
    28,
    20
  ]
}
{
  "Dep": "计算机学院",
  "push_age": [
    25,
    20,
    22
  ]
}
```

此操作对记录按dep字段值进行分组，每个分组中嵌套对象age字段的值使用$push放入数组中返回，输出字段名为 push_age，如下：

##$sum##

记录分组后，返回每个分组中指定字段值的总和，对嵌套对象使用点操作符（.）引用字段名。

###示例###

对记录分组后，返回分组中指定字段值的总和：

```lang-javascript
> db.foo.bar.aggregate({ $group: { _id: "$dep", sum_score: { $sum: "$score" }, Dep: { $first: "$dep" } } })
{
  "sum_score": 888,
  "Dep": "物电学院"
}
{
  "sum_score": 476,
  "Dep": "计算机学院"
}
```

此操作对记录按dep字段分组，使用$sum返回每个分组中score字段值的总和，输出字段名为sum_score；又使用$first取每个分组中第一条记录的dep字段值，输出字段名为Dep。

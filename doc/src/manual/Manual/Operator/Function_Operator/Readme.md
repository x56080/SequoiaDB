函数操作可以对字段进行函数运算。当指定多个函数操作时，支持流水线式处理，多个函数流水线执行。该操作符可以作为选择符使用，也可以搭配[匹配符][overview]使用，以实现更复杂的查询操作。

- 作为选择符使用，对查询结果进行函数运算

    查询集合 sample.employee 中的记录，并删除字段 a 取值两侧的空格，再将其转换为大写

    ```lang-javascript
    > db.sample.employee.find({},{a:{$trim:1, $upper:1}})
    ```

- 搭配匹配符使用，先对字段值进行函数运算，再将运算结果作为匹配条件进行查询

    查询集合 sample.employee 中字段 a 字节数为 3 的记录

    ```lang-javascript
    > db.sample.employee.find({a:{$strlen:1, $et:3}})
    ```

>**Note**
>
> 当字段类型为数组时，函数将对数组中的每个元素进行函数运算。

所支持的函数操作如下：

| 函数                        | 描述                 | 示例                                     |
| --------------------------- | -------------------- | ---------------------------------------- |
| [$abs][abs]                 | 取绝对值             | db.sample.employee.find({}, {a:{$abs:1}}) |
| [$ceiling][ceiling]         | 向上取整             | db.sample.employee.find({}, {a:{$ceiling:1}}) |
| [$floor][floor]             | 向下取整             | db.sample.employee.find({}, {a:{$floor:1}}) |
| [$round][round]             | 四舍五入为整数或指定的小数位 | db.sample.employee.find({}, {a:{$round:1}}) |
| [$format][format]           | 舍入至指定小数位并格式化 | db.sample.employee.find({}, {a:{$format:1}}) |
| [$mod][mod]                 | 取模运算             | db.sample.employee.find({}, {a:{$mod:1}}) |
| [$add][add]                 | 加法运算             | db.sample.employee.find({}, {a:{$add:10}}) |
| [$subtract][subtract]       | 减法运算             | db.sample.employee.find({}, {a:{$subtract:10}}) |
| [$multiply][multiply]       | 乘法运算             | db.sample.employee.find({}, {a:{$multiply:10}}) |
| [$divide][divide]           | 除法运算             | db.sample.employee.find({}, {a:{$divide:10}}) |
| [$substr][substr]           | 截取指定字节数的子串<br>v3.6.1 及以上版本中，该操作符已更名为 $substrBytes | db.sample.employee.find({}, {a:{$substr:[0, 4]}}) |
| [$substrBytes][substrBytes] | 截取指定字节数的子串 | db.sample.employee.find({}, {a:{$substrBytes:[0, 4]}}) |
| [$substrCP][substrCP]       | 截取指定字符数的子串 | db.sample.employee.find({}, {a:{$substrCP:[0, 4]}}) |
| [$leftBytes][rightBytes]    | 从字符串开头截取指定字节数的子串 | db.sample.employee.find({}, {a:{$leftBytes:4}}) |
| [$leftCP][rightCP]          | 从字符串开头截取指定字符数的子串 | db.sample.employee.find({}, {a:{$leftCP:4}}) |
| [$rightBytes][rightBytes]   | 从字符串末尾截取指定字节数的子串 | db.sample.employee.find({}, {a:{$rightBytes:4}}) |
| [$rightCP][rightCP]         | 从字符串末尾截取指定字符数的子串 | db.sample.employee.find({}, {a:{$rightCP:4}}) |
| [$strlen][strlen]           | 获取指定字段的字节数 | db.sample.employee.find({}, {a:{$strlen:10}}) |
| [$strlenBytes][strlenBytes] | 获取指定字段的字节数 | db.sample.employee.find({}, {a:{$strlenBytes:10}}) |
| [$strlenCP][strlenCP]       | 获取指定字段的字符数 | db.sample.employee.find({}, {a:{$strlenCP:10}}) |
| [$lower][lower]             | 字符串转为小写       | db.sample.employee.find({}, {a:{$lower:1}}) |
| [$upper][upper]             | 字符串转为大写       | db.sample.employee.find({}, {a:{$upper:1}}) |
| [$ltrim][ltrim]             | 去除左侧空格         | db.sample.employee.find({}, {a:{$ltrim:1}}) |
| [$rtrim][rtrim]             | 去除右侧空格         | db.sample.employee.find({}, {a:{$rtrim:1}}) |
| [$trim][trim]               | 去除左右两侧空格     | db.sample.employee.find({}, {a:{$trim:1}}) |
| [$cast][cast]               | 转换字段类型         | db.sample.employee.find({}, {a:{$cast:"int32"}}) |
| [$size][size]               | 获取数组元素个数     | db.sample.employee.find({}, {a:{$size:1}}) |
| [$type][type]               | 获取字段类型         | db.sample.employee.find({}, {a:{$type:1}}) |
| [$slice][slice]             | 截取数组元素         | db.sample.employee.find({}, {a:{$slice:[0,2]}}) |
| [$concat][concat]           | 连接字符串           | db.sample.employee.find({}, {a:{$concat:"abc"}}) |
| [$year][year]               | 获取日期时间中的年份  | db.sample.employee.find({}, {a:{$year: 1}}) |
| [$month][month]             | 获取日期时间中的月份  | db.sample.employee.find({}, {a:{$month: 1}}) |
| [$day][day]                 | 获取日期时间中的天数  | db.sample.employee.find({}, {a:{$day: 1}}) |


[^_^]:
    本文使用的所有引用及链接
[overview]:manual/Manual/Operator/Match_Operator/Readme.md
[Selector_Operator]:manual/Manual/Operator/Selector_Operator/Readme.md
[abs]:manual/Manual/Operator/Function_Operator/abs.md
[ceiling]:manual/Manual/Operator/Function_Operator/ceiling.md
[floor]:manual/Manual/Operator/Function_Operator/floor.md
[round]:manual/Manual/Operator/Function_Operator/round.md
[mod]:manual/Manual/Operator/Function_Operator/mod.md
[add]:manual/Manual/Operator/Function_Operator/add.md
[subtract]:manual/Manual/Operator/Function_Operator/subtract.md
[multiply]:manual/Manual/Operator/Function_Operator/multiply.md
[divide]:manual/Manual/Operator/Function_Operator/divide.md
[substr]:manual/Manual/Operator/Function_Operator/substr.md
[substrCP]:manual/Manual/Operator/Function_Operator/substrCP.md
[substrBytes]:manual/Manual/Operator/Function_Operator/substrBytes.md
[rightCP]:manual/Manual/Operator/Function_Operator/rightCP.md
[rightBytes]:manual/Manual/Operator/Function_Operator/rightBytes.md
[leftCP]:manual/Manual/Operator/Function_Operator/leftCP.md
[leftBytes]:manual/Manual/Operator/Function_Operator/leftBytes.md
[strlen]:manual/Manual/Operator/Function_Operator/strlen.md
[lower]:manual/Manual/Operator/Function_Operator/lower.md
[upper]:manual/Manual/Operator/Function_Operator/upper.md
[ltrim]:manual/Manual/Operator/Function_Operator/ltrim.md
[rtrim]:manual/Manual/Operator/Function_Operator/rtrim.md
[trim]:manual/Manual/Operator/Function_Operator/trim.md
[cast]:manual/Manual/Operator/Function_Operator/cast.md
[size]:manual/Manual/Operator/Function_Operator/size.md
[type]:manual/Manual/Operator/Function_Operator/type.md
[slice]:manual/Manual/Operator/Function_Operator/slice.md
[strlenBytes]:manual/Manual/Operator/Function_Operator/strlenBytes.md
[strlenCP]:manual/Manual/Operator/Function_Operator/strlenCP.md
[concat]:manual/Manual/Operator/Function_Operator/concat.md
[format]:manual/Manual/Operator/Function_Operator/format.md
[year]:manual/Manual/Operator/Function_Operator/year.md
[month]:manual/Manual/Operator/Function_Operator/month.md
[day]:manual/Manual/Operator/Function_Operator/day.md
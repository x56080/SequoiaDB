使用访问计划缓存可以减少查询优化器生成访问计划的开销，提高查询的效率。

##基本描述##

目前 SequoiaDB 提供多个访问计划的缓存级别，可以通过 SequoiaDB 的 --plancachelevel 参数进行设置：

| 取值      | 对应内部值             | 解释 |
| --------- | ---------------------- | ---- |
| 0         | OPT_PLAN_NOCACHE       | 不使用访问计划缓存 |
| 1         | OPT_PLAN_ORIGINAL      | 对缓存的访问计划进行精确匹配，即使用匹配符、排序字段、索引提示等进行直接比较 |
| 2         | OPT_PLAN_NORMALIZED    | 对缓存的访问计划进行模糊匹配，允许改变匹配符的顺序 |
| 3（默认） | OPT_PLAN_PARAMETERIZED | 对缓存的访问计划进行模糊匹配，允许改变匹配符的顺序，允许参数绑定 |
| 4         | OPT_PLAN_FUZZYOPTR     | 对缓存的访问计划进行模糊匹配，允许改变匹配符的顺序，允许参数绑定、允许某些相似的匹配符（如：$gt 和 $gte 可以重用） |

>   **Note:**
>
>   * 访问计划缓存的 SequoiaDB 配置参数 planbuckets，详细请参考[数据库配置](database_management/database_configuration/configuration_parameters.md)
>        * planbuckets 定义全局的访问计划缓存的桶的个数，默认为 500，最大值为 4096
>        * SequoiaDB 内部自动向上取整为 0, 128, 256, 512, 1024, 2048, 4096
>        * planbuckets 设置为 0 时，不开启访问计划缓存
>        * 每个缓存桶中平均存储 3 个访问计划缓存，因此访问计划缓存的容量约为 `planbuckets * 3`
>   * 当访问计划缓存的容量使用满 80% 时，后台任务将开始清理最近最少使用的访问计划，直至访问计划缓存的使用量降到 50% 左右
>   * 另外，访问计划的缓存有效时间为 10 分钟，如果 10 分钟内没有被再次使用，则会被后台清理

###OPT_PLAN_NOCACHE###

没有使用缓存。

###OPT_PLAN_ORIGINAL###

使用缓存，匹配符必须完全匹配才可重用。

**示例**

查询 ```db.foo.bar.find( { a : 100, b : 200 } )``` 和 ```db.foo.bar.find( { b : 200, a : 100 } )``` 不可以共享访问计划

###OPT_PLAN_NORMALIZED###

使用缓存，匹配符经过排序、剪枝、合并后一致。

**示例**

查询 ```db.foo.bar.find( { a : { $gt : 100 }, b : { $et : 'a' } } )``` 和 ```db.foo.bar.find( { b : { $et : 'a' }, a : { $gt : 100 } } )``` 可以共享访问计划

###OPT_PLAN_PARAMETERIZED###

使用缓存，匹配符经过参数化后一致。

>   **Note:**
>
>   *   支持参数化的匹配符：$et、$gt、$gte、$lt、$lte、$in
>   *   $et、$gt、$gte、$lt 和 $lte 支持参数化的匹配符条件指定的值的类型：整数、长整数、浮点数、高精度数、字符串、对象 ID（OID）、日期、时间戳
>   *   $in 支持以整个数组作为参数
>   *   含有不支持参数化的匹配符将把该查询的访问计划的缓存级别降为 OPT_PLAN_NORMALIZED

**示例**

查询 ```db.foo.bar.find( { a : { $gt : 100 }, b : { $et : 'a' } } )``` 和 ```db.foo.bar.find( { a : { $gt : 200 }, b : { $et : 'b' } } )``` 可以共享访问计划

1.  参数化：```{ a : { $gt : { $param : 0, $ctype : 10 } }, { b : { $et : { $param : 1, $ctype : 15 } }```
2.  参数：```[ 100, 'a' ]``` 和 ```[ 200, 'b' ]```
3.  $param 为参数在参数列表中的位置
4.  $ctype 为参数的[比较优先级](data_model/datatype/datatype.md)，整数是 10，字符串是 15

###OPT_PLAN_FUZZYOPTR###

使用缓存，匹配符经过参数化后一致，某些操作符可以重用：$gt 和 $gte，$lt 和 $lte

>   **Note:**
>
>   支持重用的匹配符为
>
>   *   $gt 和 $gte
>   *   $lt 和 $lte

**示例**

查询 ```db.foo.bar.find( { a : { $gt : 100 } } )``` 和 ```db.foo.bar.find( { a : { $gte : 200 } } )``` 可以共享访问计划

1.  参数化：```{ a : { $gt : { $param : [ 0, 1 ], $ctype : 10 } }```
2.  参数：```[ 100, false ]``` 和 ```[ 200, true ]```
3.  $param 为参数在参数列表中的位置，及操作符本身的配置（生成的参数表示是否需要相等比较）
4.  $ctype 为参数的[比较优先级](data_model/datatype/datatype.md)

##访问计划缓存快照##

访问计划缓存快照 [SDB_SNAP_ACCESSPLANS](database_management/monitoring/snapshot/SDB_SNAP_ACCESSPLANS.md) 列出数据库中缓存的访问计划的信息。

##语法##

***db.setSessionAttr ( \<options\> )***

设置会话属性。

## 参数描述##

| 参数名    | 参数类型  | 描述          | 是否必填   |
| --------- | --------- | ------------- | ---------- |
| options   | Json 对象 | 会话属性选项  | 是         |

###options 格式###

| 属性名 | 描述      | 格式      |
| ------ | --------- | --------- |
| PreferedInstance | 会话读操作优先选择的实例，取值列表："M"、"m"、"S"、"s"、"A"、"a"、1-255。可以使用数组指定多个取值。<br>"M", "m"：可读写实例（主实例）<br>"S", "s"：只读实例（备实例）<br>"A", "a"：任意实例<br>1-255：通过 --instanceid 指定实例 ID 的实例。 | ```PreferedInstance : "M"```<br>```PreferedInstance : [ 1, 10 ]``` |
| PreferedInstanceMode | 指定会话当多个实例符合 PreferedInstance 的条件时的选择模式。<br>"random"：从候选的实例取值中随机选择。<br>"ordered"：从候选的实例取值中按照 PerferedInstance 的顺序进行选择。<br>PreferedInstance 中的角色取值，根据规则选择时优于或者次于实例取值，与 PreferedInstanceMode 取值无关。 | ```PreferedInstaceMode : "random"``` |
| PreferedStrict   |  指定节点选择是否为严格模式，当为严格模式时，节点只能从 preferedinstance 指定的ID中选取 |  ```PreferedStrict : true ``` |
| PreferedPeriod   | 优先实例的有效周期，单位为秒。 | ```PerferedPeriod : 60``` |
| Timeout | 指定会话执行操作的超时时间（单位：毫秒）。<br>-1 表示不进行超时检测。<br>最小值为 1000 毫秒。 | ```Timeout : 10000``` |
| TransIsolation | 会话事务的隔离级别，0为RU级别，1为RC级别，2为RS级别。 | ```TransIsolation : 1``` |
| TransTimeout   | 会话事务锁等待超时时间（单位：秒）。 | ```TransTimeout : 10``` |
| TransLockWait  | 会话事务在RC隔离级别下是否需要等锁。 | ```TransLockWait : true``` |
| TransUseRBS    | 会话事务是否使用回滚段。             | ```TransUseRBS : true``` |
| TransAutoCommit| 会话事务是否支持自动事务提交。       | ```TransAutoCommit : true``` |
| TransAutoRollback | 会话事务在操作失败时是否自动回滚。 | ```TransAutoRollback : true``` |
| TransRCCount | 会话事务是否使用读已提交来处理 count() 查询。 | ```TransRCCount : true ``` |

>   **Note:**
>
>   *   PreferedInstance 和 PreferedInstaceMode 的缺省值是协调节点配置中 --preferedinstance 和 --preferedinstancemode 的取值。
>       *   协调节点配置 --preferedinstance 的默认值是 "M"，--preferedinstacemode 的默认值是 "random"。
>    *   PreferedInstance 的取值分为两类，一类是角色取值，如 "M", "S" 等；一类是实例取值，即数据节点通过配置 instanceid 设置的实例 ID。
>       *   角色取值："M", "m"：可读写实例（主实例）；"S", "s": 只读实例（备实例）；"A", "a": 任意角色实例。
>       *   实例取值：1-255，指定匹配 instanceid 设置的节点。数据节点可以通过配置 instanceid 配合使用。
>          *   实例的 ID 可以通过数据节点的配置项 --instanceid 进行设置，同一个数据组中可以配置多个相同实例 ID 的数据节点。
>          *   修改数据节点的配置项 --instanceid 不能动态生效，需要手工停启数据节点。重启之后也需要手工调用 [Sdb.invalidateCache()](reference/Sequoiadb_command/Sdb/invalidateCache.md) 清空各个协调节点的缓存。
>          *   在节点配置了instanceid的情况下，按照instanceid进行获取。在节点没有配置instanceid的情况下，按照节点的nodeid在组内的排序序列（从1开始）作为instanceid来进行选取，例如 组 db1 中有3个节点 [ { NodeID:1001}, {NodeID:1004}, {NodeID:1002} ]，那么其节点的 instanceid 分别为 1, 3, 2。
>       *   如果一个或多个实例取值和一个角色取值混合指定，则优先选择匹配实例 ID 的一组节点中符合角色的节点。如 ```[ 1, 2, "S" ]``` 表示优先选择实例 ID 为 1 或者 2 的节点中的备节点。
>       *   混合指定时，可以通过 "-" 扩展角色取值，如 "-M"，"-S" 等，表示如果没有匹配实例 ID 的节点时，优先选择角色节点，如 ```[ 1, 2, "-S" ]``` 表示优先匹配实例 ID 为 1 或者 2 的节点，如果没有，则优先选择任意一个备节点。
>       *   混合指定时，如果指定多个角色取值实例，或者其扩展取值，则只有第一个生效。如 ```[ "M", "S" ]``` 中只生效 "M"。
>       *   单独指定时，角色取值的扩展模式和角色取值的语义是相同的。如单独指定 "S" 和 "-S"，语义是一致的。
>       *   如果同一个会话中，读请求前有写请求，写请求之后的一段时间内读请求将默认使用写请求使用的节点（可读写例）进行读取，可以通过设置 PreferedPeriod 来修改读请求复用写请求节点的有效期限。
>       *   如果没有符合 PreferedInstance 的实例，之前也没有写请求，一般在数据组中随机选取节点进行。特殊情况是，为了兼容之前的版本，如果单独指定一个实例取值时，将按照实例ID - 1后对节点总数取模后在组内按照nodeid的升序排序顺序选取。
>   * PreferedPeriod 的缺省值是协调节点配置中 --preferedperiod 的取值。
>       *   如果上一次选择进行请求的节点在有效周期内，读请求仍使用该节点进行查询，周期之后，将根据 PreferedInstance 重新选择。
>       *   默认值为60。
>       *   取值范围为 [-1, 2^31]。
>       *   -1表示不失效。
>       *   0表示本次查询不使用上次选择的优先实例，根据 PreferedInstance 进行重新选择。
>   *   Timeout 的默认值是 -1，即不进行超时检测。
>   *   事务相关属性只有 TransTimeout 允许在事务中设置，其它事务属性需要在非事务中设置。
>   *   获取会话属性请参考 [Sdb.getSessionAttr()](reference/Sequoiadb_command/Sdb/getSessionAttr.md) 。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。

*   **SDB_INVALIDARG**( -6 )
    options 属性的输入错误，请检查设置属性的值和范围等。

更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##示例##

* 设置会话优先从“主”数据库实例获取数据

```lang-javascript
> db.setSessionAttr( { PreferedInstance: "M" } )
```

* 设置会话优先从 1 和 3 的备实例读取数据

```lang-javascript
> db.setSessionAttr( { PreferedInstance : [ 1, 3, "S" ] } )
```

* 设置会话的操作超时时间为 10 秒

```lang-javascript
> db.setSessionAttr( { Timeout : 10000 } )
```

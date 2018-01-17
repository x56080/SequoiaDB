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
| PreferedInstance | 会话读操作优先选择的实例，取值列表："M"、"m"、"S"、"s"、"A"、"a"、1-255。可以使用数组指定多个取值。<br>"M", "m"：可读写实例（主实例）<br>"S", "s"：只读实例（备实例）<br>"A", "a"：任意实例<br>1-255：通过 --instanceid 指定实例 ID 的实例 | ```PreferedInstance : "M"```<br>```PreferedInstance : [ 1, 10 ]``` |
| PreferedInstanceMode | 指定会话当多个实例符合 PreferedInstance 的条件时的选择模式。<br>"random"：从候选的实例中随机选择。<br>"ordered"：从候选的实例中按照 PerferedInstance 的顺序进行选择。 | ```PreferedInstaceMode : "random"``` |
| Timeout | 指定会话执行操作的超时时间（单位：毫秒），-1 表示不进行超时检测。 | ```Timeout : 10000``` |

>   **Note:**
>
>   *   PreferedInstance 和 PreferedInstaceMode 的缺省值是协调节点配置中 --preferedinstance 和 --preferedinstancemode 的取值。
>       *   协调节点配置 --preferedinstance 的默认值是 "M"，--preferedinstacemode 的默认值是 "random"。
>       *   实例的 ID 可以通过数据节点的配置项 --instanceid 进行设置，同一个数据组中可以配置多个相同实例 ID 的数据节点。
>       *   修改数据节点的配置项 --instanceid 不能动态生效，需要手工停启数据节点。重启之后也需要手工调用 [Sdb.invalidateCache()](reference/Sequoiadb_command/Sdb/invalidateCache.md) 清空各个协调节点的缓存。
>       *   如果多个 1-255 的实例和 "M" 一起指定，则满足指定实例中的主实例会优先选择；如果多个 1-255 的实例和 "M" 或 "m" 一起指定，则当没有满足指定的实例时选择主实例。如 [ 1, 2, "M" ] 表示优先从实例 1 和实例 2 中的主实例读取；如果不存在实例 1 和实例 2，则从主实例读取。
>       *   如果多个 1-255 的实例和 "S" 一起指定，则满足指定实例中的备实例会被优先选择；如果多个 1-255 的实例和 "S" 或 "s" 一起指定，则当没有满足指定的实例时选择备实例。如 [ 1, 2, "S" ] 表示优先从实例 1 和实例 2 中的备实例读取；如果不存在实例 1 和实例 2，则从任意一个备实例读取。
>       *   如果指定多个 "M"、"m"、"S"、"s"、"A"、"a" 实例，则只有第一个生效。
>       *   如果没有符合 PreferedInstance 的实例，会话将随机选择使用上一次写操作的实例，即可读写（主）实例进行查询（如无写操作，则随机选取实例）。
>   *   Timeout 的默认值是 -1，即不进行超时检测。

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

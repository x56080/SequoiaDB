##名称##

setSessionAttr - 设置会话属性

##语法##

**db.setSessionAttr(\<options\>)**

##类别##

Sdb

##描述##

该函数用于设置会话属性。

##参数##

options（ *object，必填* ）

通过该参数可以设置会话的属性

- PreferedInstance（ *string/array/number* ）：会话读操作优先选择的实例

    用户可以使用数组指定多个取值，具体取值如下：
    - "M", "m"：可读写实例（主实例）
    - "S", "s"：只读实例（备实例）
    - "A", "a"：任意实例
    - 1~255：通过 instanceid 指定实例 ID

    格式：`PreferedInstance : "M"` 或 `PreferedInstance : [ 1, 10 ]`

- PreferedInstanceMode（ *string* ）：指定会话当多个实例符合 PreferedInstance 的条件时的选择模式

    该参数取值如下：
    - "random"：从候选的实例取值中随机选择
    - "ordered"：从候选的实例取值中按照 PerferedInstance 的顺序进行选择

    PreferedInstance 中的角色取值，根据规则选择时优于或者次于实例取值，与 PreferedInstanceMode 取值无关。

    格式：`PreferedInstaceMode : "random"`

- PreferedStrict（ *boolean* ）：指定节点选择是否为严格模式

    当指定为严格模式时，节点只能从 preferedinstance 指定的 ID 中选取。

    格式：`PreferedStrict : true `

- PreferedPeriod（ *number* ）：指定优先实例的有效周期，单位为秒

    格式：`PreferedPeriod : 60`

- Timeout（ *number* ）：指定会话执行操作的超时时间，单位为毫秒

    该参数最小取值为 1000 毫秒；取值为 -1 表示不进行超时检测。

    格式：`Timeout : 10000`

- TransIsolation（ *number* ）：会话事务的隔离级别

    该参数取值如下：
    - 0：表示 RU 级别
    - 1：表示 RR 级别
    - 2：表示 RS 级别

    格式：`TransIsolation : 1`

- TransTimeout（ *number* ）：会话事务锁等待超时时间，单位为秒

    格式：`TransTimeout : 10`

- TransLockWait（ *boolean* ）：会话事务在 RC 隔离级别下是否需要等锁

    格式：`TransLockWait : true`

- TransUseRBS（ *boolean* ）：会话事务是否使用回滚段

    格式：`TransUseRBS : true`

- TransAutoCommit（ *boolean* ）：会话事务是否支持自动事务提交

    格式：`TransAutoCommit : true`

- TransAutoRollback（ *boolean* ）：会话事务在操作失败时是否自动回滚

    格式：`TransAutoRollback : true`

- TransRCCount（ *boolean* ）：会话事务是否使用读已提交来处理 count() 查询

    格式：`TransRCCount : true`

- TransMaxLockNum（ *number* ）：会话事务在一个数据节点上可以持有最大的记录锁个数，默认值为 10000，取值范围为[-1, 2^31 - 1]。

    该参数取值为 -1 时，表示事务对记录锁的个数无限制；取值为 0 时，表示事务不使用记录锁，直接使用集合锁。

    格式：`TransMaxLockNum : 10000`

- TransAllowLockEscalation（ *boolean* ）：会话事务持有记录锁个数超过参数 TransMaxLockNum 设置的值后，是否允许锁升级，默认值为 true，表示允许锁升级。

    如果事务持有的记录锁个数达到上限，但该参数的值为 false，事务操作将报错。

    格式：`TransAllowLockEscalation : true`

- TransMaxLogSpaceRatio（ *number* ）：会话事务在一个数据节点上可以使用的最大日志空间比例(%)，默认值为 50，取值范围为[1, 50]。

    该参数表示事务占数据节点日志空间总大小（日志空间总大小=logfilesz*logfilenum）的最大百分比。当事务使用的日志空间达到上限时，事务操作将报错。

    格式：`TransMaxLogSpaceRatio : 50`

> **Note:**
>
> * PreferedInstance 和 PreferedInstaceMode 的缺省值是协调节点配置中 preferedinstance 和 preferedinstancemode 的取值。
>    * 协调节点配置 preferedinstance 的默认值是 "M"，preferedinstacemode 的默认值是 "random"。
> * PreferedInstance 的取值分为两类，一类是角色取值，如 "M"、"S" 等；一类是实例取值，即数据节点通过配置 instanceid 设置的实例 ID。
>     1. 角色取值："M"、"m"：可读写实例（主实例）；"S"、"s": 只读实例（备实例）；"A"、"a": 任意角色实例。
>     2. 实例取值：1~255，指定匹配 instanceid 设置的节点。数据节点可以通过配置 instanceid 配合使用。
>         * 实例的 ID 可以通过数据节点的配置项 instanceid 进行设置，同一个数据组中可以配置多个相同实例 ID 的数据节点。
>         * 修改数据节点的配置项 instanceid 不能动态生效，需要手工停启数据节点。重启之后也需要手工调用 [Sdb.invalidateCache()][invalidateCache] 清空各个协调节点的缓存。
>         * 在节点配置了 instanceid 的情况下，按照 instanceid 进行获取。在节点没有配置 instanceid 的情况下，按照节点的 NodeID 在组内的排序序列（从 1 开始）作为 instanceid 来进行选取，例如组 db1 中有三个节点 [ {NodeID: 1001}, {NodeID: 1004}, {NodeID: 1002} ]，那么其节点的 instanceid 分别为 1、3、2。
>     3. 如果一个或多个实例取值和一个角色取值混合指定，则优先选择匹配实例 ID 的一组节点中符合角色的节点。如 ```[ 1, 2, "S" ]``` 表示优先选择实例 ID 为 1 或者 2 的节点中的备节点。
>     4. 混合指定时，可以通过 "-" 扩展角色取值，如 "-M"，"-S" 等，表示如果没有匹配实例 ID 的节点时，优先选择角色节点，如 ```[ 1, 2, "-S" ]``` 表示优先匹配实例 ID 为 1 或者 2 的节点，如果没有，则优先选择任意一个备节点。
>     5. 单独指定时，角色取值的扩展模式和角色取值的语义是相同的。如单独指定 "S" 和 "-S"，语义是一致的。
>     6. 如果同一个会话中，读请求前有写请求，写请求之后的一段时间内，读请求将默认使用写请求使用的节点（可读写实例）进行读取，可以通过设置 PreferedPeriod 来修改读请求复用写请求节点的有效期限。
>     7. 如果没有符合 PreferedInstance 的实例，之前也没有写请求，一般在数据组中随机选取节点进行。特殊情况是，为了兼容之前的版本，如果单独指定一个实例取值时，将按照实例 ID-1 后对节点总数取模，再在组内按照 NodeID 的升序排序顺序选取。
> * PreferedPeriod 的缺省值是协调节点配置中 preferedperiod 的取值。
>     * 如果上一次选择进行请求的节点在有效周期内，读请求仍使用该节点进行查询。周期之后，将根据 PreferedInstance 重新选择。
>     * 默认值为 60。
>     * 取值范围为 [-1, 2^31 - 1]。
>     * -1 表示不失效。
>     * 0 表示本次查询不使用上次选择的优先实例，根据 PreferedInstance 进行重新选择。
>     * 该参数仅适用于 SequoiaDB 2.8.9 版本，3.2.5 及以上版本。
> * Timeout 的默认值是 -1，即不进行超时检测。
> * 事务相关属性只有 TransTimeout 允许在事务中设置，其它事务属性需要在非事务中设置。
> * 获取会话属性可参考 [Sdb.getSessionAttr()][getSessionAttr]。

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`setSessionAttr()` 函数常见异常如下：

| 错误码 | 错误类型       | 可能发生的原因       | 解决办法                   |
|--------|----------------|----------------------|----------------------------|
| -6     | SDB_INVALIDARG | options 属性输入错误 | 检查所设置属性的值和范围等 |

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取[错误码][error_code]，关于错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* 设置会话优先从“主”数据库实例获取数据

    ```lang-javascript
    > db.setSessionAttr({PreferedInstance: "M"})
    ```

* 设置会话优先从 1 和 3 的备实例读取数据

    ```lang-javascript
    > db.setSessionAttr({PreferedInstance: [ 1, 3, "S" ]})
    ```

* 设置会话的操作超时时间为 10 秒

    ```lang-javascript
    > db.setSessionAttr({Timeout: 10000})
    ```


[^_^]:
    本文使用的所有引用及链接
[invalidateCache]:manual/Manual/Sequoiadb_Command/Sdb/invalidateCache.md
[getSessionAttr]:manual/Manual/Sequoiadb_Command/Sdb/getSessionAttr.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md

[^_^]:
    元数据一致性检测工具

sdbconsistencycheck 是 SequoiaDB 检测集群元数据一致性的工具

在 SequoiaDB v3.6/5.0.3 及以上版本创建索引时，编目节点上会添加索引的元数据信息并生成 UniqueID。因此，用户将 SequoiaDB 升级至 v3.6/5.0.3 及以上版本后，需手动执行 sdbconsistencycheck 工具检测集群元数据一致性，并执行索引升级脚本。

##语法规则##

```lang-text
sdbconsistencycheck [ options ] ...
```

##参数说明##

- **--help, -h**

    获取帮助信息

- **--version**

    获取版本信息

- **--hostname, -s \<coord hostname\>**

    指定协调节点所在的主机名

- **--svcname, -p \<coord port\>**

    指定协调节点的端口号

- **--username, -u \<user name\>**

    指定用户名，默认为空字符串

- **--password, -w \<password\>**

    指定用户密码，默认为空字符串

- **--cipher \<boolean\>**

    是否使用密文模式输入密码，默认为false，取值如下：

    - "true": 使用密文模式输入密码，配合 --cipherfile --token 参数使用，关于密文模式的介绍可参考[密码管理][passwd]

    - "false": 使用明文模式 --password 输入密码

- **--cipherfile \<cipher file\>**

    指定密文文件路径，默认值为 ~/sequoiadb/passwd

- **--token \<token\>**

    指定密文文件的加密令牌

    如果创建密文文件时未指定 token，可忽略该参数

- **--output, -o \<output file\>**

    指定输出报告的文件路径，默认输出在当前路径下的 `sdbconsistencycheck.log` 文件中

- **--sdbShellPath \<path\>**

    指定 sdb shell 路径，默认值为 ${installPath}/bin/sdb

- **--action \<action\>**

    指定操作，取值如下：

    - "clear": 删除 check 操作过程中生成的临时表和 generate 操作过程中生成的代码文件；

    - "check": 检测集群元数据一致性，并生成检测报告；

    - "generate": 根据元数据一致性信息，生成 js 脚本，代码生成策略见配置文件说明。脚本存放路径为当前目录下的 js 目录。目前该操作会生成 5 类脚本，脚本可以直接用 sdb shell 执行，分别是：

       - upgradeIndexes.js，索引升级脚本；

       - missIndexes.js，缺失索引删除/重建脚本；

       - conflictIndexes.js，冲突索引删除/重建脚本；

       - invalidIdIndexes.js，无效 $id 索引删除脚本；

       - invalidCls.js，无效集合删除脚本；

##配置文件说明##

- **COLLECT_DATA_INDEX_INFO_THREAD_NUM**

    配置后台收集数据节点索引信息线程数，当集合数小于等于进程数时，只会开启一个线程收集索引信息

- **DEALWITH_MISS_INDEX**

    缺失索引清理代码生成策略（涉及删除的代码生成时都加了注释，执行时需要手工确认并删除注释代码），取值如下：

    - DROP_STRATEGY: 生成删除索引的代码；

    - RE_CREATE_STRATEGY: 生成重建索引的代码；

    - IGNORE_STRATEGY: 不生成任何代码；

- **DEALWITH_CONFLICT_INDEX**

    冲突索引清理代码生成策略（涉及删除的代码生成时都加了注释，执行时需要手工确认并删除注释代码），取值如下：

    - DROP_STRATEGY: 生成删除索引的代码；

    - RE_CREATE_STRATEGY: 生成重建索引的代码；

    - IGNORE_STRATEGY: 不生成任何代码；

- **DEALWITH_LOCAL_CL**

    无效集合清理代码生成策略（涉及删除的代码生成时都加了注释，执行时需要手工确认并删除注释代码），取值如下：

    - DROP_STRATEGY: 生成删除集合的代码；

    - IGNORE_STRATEGY: 不生成任何代码；

##示例##

下述以协调节点 `localhost:11810`，工具所在路径 `/opt/sequoiadb/tools/consistencycheck` 为例，演示索引升级步骤。

1. 检验元数据信息

    ```lang-bash
    $ ./sdbconsistencycheck -s localhost -p 11810 --action check
    ```

2. 查看检验报告，获取当前集群元数据一致性情况

    ```lang-bash
    $ vi sdbconsistencycheck.log
    ```

3. 生成 js 脚本

    ```lang-bash
    $ ./sdbconsistencycheck -s localhost -p 11810 --action generate
    ```

4. 确认 js 脚本中的删除索引/集合代码，手工删除注释代码

5. 执行 js 脚本，升级索引，清理不一致索引和清理残留集合

    ```lang-bash
    $ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/upgradeIndexes.js
    $ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/missIndexes.js
    $ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/conflictIndexes.js
    $ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/invalidIdIndexes.js
    $ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/invalidCls.js
    ```

6. 重新检验元数据信息

    ```lang-bash
    $ ./sdbconsistencycheck -s localhost -p 11810 --action check
    $ vi sdbconsistencycheck.log
    ```

7. 确认索引都已经升级，不一致索引和无效集合都清理完毕，清理 check 和 generate 操作生成的临时表和 js 脚本

    ```lang-bash
    $ ./sdbconsistencycheck -s localhost -p 11810 --action clear
    ```

##报告解析##

###查看不需要升级的索引###

“No Need to Upgrade”表示索引均已存在合法的 UniqueID，无需升级。

```lang-text
===================== Check Result ( No Need to Upgrade ) ======================
 ID       Collection IndexName  IndexType
  1  sample.employee       $id Consistent
  2  sample.employee   nameIdx Consistent
```

IndexType 取值如下：

* Consistent：一致性索引，表示在编目节点和各数据节点上均有索引 UniqueID
* Standalone：[独立索引][standalone]

###查看可升级的索引###

“Can be Upgraded”表示索引在集合的各数据节点上定义一致，但缺失 UniqueID，需要升级

```lang-text
===================== Check Result ( Can be Upgraded ) =========================
 ID      Collection IndexName                IndexAttr IndexKey
  1  sample.January       $id Unique|Enforced|NotArray {"_id":1}
  2  sample.January   nameIdx                        - {"name":1}
  3  sample.January    ageIdx                   Unique {"age":1}
```

执行 generate 操作生成的脚本，升级索引

```lang-bash
$ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/upgradeIndexes.js
```

索引升级时可能会受到其他业务操作、环境问题（如网络问题）的影响，出现少量索引升级失败。此时请根据错误信息与 SequoiaDB 集群的日志信息排查问题，然后重新执行升级脚本。

###查看不可升级的索引###

“Cannot be Upgraded”表示索引均不可升级，具体不可升级的原因及解决办法可参考后续说明。

```lang-text
===================== Check Result ( Cannot be Upgraded ) ======================
 ID             Reason       Collection IndexName                IndexAttr IndexKey
  1            Missing  sample.February   nameIdx                        - {"name":1}
  2           Conflict  sample.February    ageIdx                        - {"age":1}
  2           Conflict  sample.February    ageIdx                        - {"age1":1}
  3         Invalid CL       sample.May       $id Unique|Enforced|NotArray {"_id":1}
  4         Invalid CL      sample.June       $id Unique|Enforced|NotArray {"_id":1}
  5    ResidualIdIndex     sample.April       $id Unique|Enforced|NotArray {"_id":1}

  ---------- Index ( ID: 1 ) Missing -----------
             GroupName           NodeName Missing
                group2   sdbserver1:11830       Y
                group2   sdbserver2:11830       N
                group2   sdbserver3:11830       N

  ---------- Index ( ID: 2 ) Conflict -----------
             IndexName   IndexKey IndexAttr GroupName  NodeName
                ageIdx  {"age":1}         -    group2  sdbserver1:11830,sdbserver2:11830
                ageIdx {"age1":1}         -    group2  sdbserver3:11830

  ---------- Index ( ID: 3 ) Local -----------
             Collection NodeName
             sample.May sdbserver1:11830

  ---------- Index ( ID: 4 ) Residual -----------
             Collection NodeName
            sample.June sdbserver2:11830

  ---------- Index ( ID: 5 ) ResidualIdIndex -----------
             Collection NodeName
           sample.April sdbserver1:11830
           sample.April sdbserver2:11830
           sample.April sdbserver3:11830
```

- **索引缺失**

    “Missing”表示“Cannot be Upgraded”列表中 ID 为 1 的索引，存在于数据节点 `sdbserver1:11830`，但不存在于数据节点 `sdbserver2:11830` 和 `sdbserver3:11830`。

    ```lang-text
    ===================== Check Result ( Cannot be Upgraded ) ======================
    ID             Reason       Collection IndexName                IndexAttr IndexKey
     1            Missing  sample.February   nameIdx                        - {"name":1}

    ---------- Index ( ID: 1 ) Missing -----------
               GroupName           NodeName Missing
                  group2   sdbserver1:11830       Y
                  group2   sdbserver2:11830       N
                  group2   sdbserver3:11830       N
    ```

    解决办法：执行 sdbconsistencycheck 生成的脚本

    ```lang-bash
    $ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/missIndexes.js
    ```

- **索引冲突**

    “Conflict”表示“Cannot be Upgraded”列表中 ID 为 2 的索引与其他索引冲突。当集合名相同但索引键值或者属性不同，或当索引键值相同但索引名字或者属性不同时，均会导致索引冲突。

    ```lang-text
    ===================== Check Result ( Cannot be Upgraded ) ======================
     ID             Reason       Collection IndexName                IndexAttr IndexKey
      2           Conflict  sample.February    ageIdx                        - {"age":1}
      2           Conflict  sample.February    ageIdx                        - {"age1":1}

    ---------- Index ( ID: 2 ) Conflict -----------
               IndexName   IndexKey IndexAttr GroupName  NodeName
                  ageIdx  {"age":1}         -    group2  sdbserver1:11830,sdbserver2:11830
                  ageIdx {"age1":1}         -    group2  sdbserver3:11830
    ```

    解决办法：执行 sdbconsistencycheck 生成的脚本

    ```lang-bash
    $ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/conflictIndexes.js
    ```

- **残留集合**

    “Invalid CL”表示“Cannot be Upgraded”列表中 ID 为 3 的索引存在于节点 `sdbserver1:11830` 的残留集合 sample.May 中，残留集合类型为 “Local”，ID 为 4 的索引存在于节点 `sdbserver2:11830` 的残留集合 sample.June 中，残留集合类型为 “Residual”。

    “Local”表示直连数据节点创建的集合；

    “Residual”表示连接协调节点创建的集合，但此时编目元数据已经丢失，只有数据节点有数据。在删除集合过程中，节点异常重启，就有可能导致“Residual”类型的集合残留。

    ```lang-text
    ===================== Check Result ( Cannot be Upgraded ) ======================
     ID             Reason       Collection IndexName                IndexAttr IndexKey
      3         Invalid CL       sample.May       $id Unique|Enforced|NotArray {"_id":1}
      4         Invalid CL      sample.June       $id Unique|Enforced|NotArray {"_id":1}

    ---------- Index ( ID: 3 ) Local -----------
               Collection NodeName
               sample.May sdbserver1:11830

    ---------- Index ( ID: 4 ) Residual -----------
               Collection NodeName
              sample.June sdbserver2:11830
    ```

    解决办法：执行 sdbconsistencycheck 生成的脚本

    ```lang-bash
    $ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/invalidCls.js
    ```

- **残留 $id 索引**

    “ResidualIdIndex”表示“Cannot be Upgraded”列表中 ID 为 5 的索引是存在于数据节点的残留 $id 索引，所在集合 sample.March 的集合属性 “AutoIndexId” 为 false

    ```lang-text
    ===================== Check Result ( Cannot be Upgraded ) ======================
     ID             Reason       Collection IndexName                IndexAttr IndexKey
      5    ResidualIdIndex     sample.March       $id Unique|Enforced|NotArray {"_id":1}

    ---------- Index ( ID: 5 ) ResidualIdIndex -----------
               Collection NodeName
             sample.April sdbserver1:11830
             sample.April sdbserver2:11830
             sample.April sdbserver3:11830
    ```

    解决办法：执行 sdbconsistencycheck 生成的脚本

    ```lang-bash
    $ /opt/sequoiadb/bin/sdb -f /opt/sequoiadb/tools/consistencycheck/js/invalidIdIndexes.js
    ```

[^_^]:
    本文使用的所有引用及链接
[standalone]:manual/Distributed_Engine/Architecture/Data_Model/index.md#创建索引
[passwd]:manual/Distributed_Engine/Maintainance/Security/system_security.md#密码管理

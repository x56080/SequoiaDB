##创建集合空间##

|          | 说明                                     | 例子                                                               |
| -------- | ---------------------------------------- | ------------------------------------------------------------------ |
| 请求头   | 同通用请求头                             |                                                                    |
| 请求内容 | cmd: create collectionspace<br>name: 集合空间名字 | cmd=create collectionspace&name=foo |
| 说明     |                                          |                                                                    |
| 响应头   | 同通用响应头                             |                                                                    |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": -33, "description": "Collection space already exists" }] |
| 说明     |                                          |                                                                    |

##删除集合空间##

|          | 说明                                     | 例子                             |
|----------|------------------------------------------|----------------------------------|
| 请求头   | 同通用请求头                             |                                  |
| 请求内容 | cmd: drop collectionspace<br>name: 集合空间名字 | cmd=drop collectionspace&name=foo |
| 说明     |                                          |                                  |
| 响应头   | 同通用响应头                             |                                  |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述 <br>} | [{ "errno": 0 }] |
| 说明     |                                          |                                   |

##创建集合##

|          | 说明                                     | 例子                              |
|----------|------------------------------------------|-----------------------------------|
| 请求头   | 同通用请求头                             |                                   |
| 请求内容 | cmd: create collection<br>name: 集合的全称（集合空间.集合）<br>options: 选项（可选参数，可不填） | cmd=create collection&name=foo.bar |
| 说明     |                                          |                                   |
| 响应头   | 同通用响应头                             |                                   |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |                                          |                                   |

##删除集合##

|          | 说明                                     | 例子                            |
|----------|------------------------------------------|---------------------------------|
| 请求头   | 同通用请求头                             |                                 |
| 请求内容 | cmd: drop collection<br>name: 集合的全称（集合空间.集合） | cmd=drop collection&name=foo.bar |
| 说明     |                                          |                                 |
| 响应头   | 同通用响应头                             |                                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |                                          |                                 |

##插入数据##

|          | 说明                                     | 例子                                                      |
|----------|------------------------------------------|-----------------------------------------------------------|
| 请求头   | 同通用请求头                             |                                                           |
| 请求内容 | cmd: insert<br>name: 集合的全称（集合空间.集合）<br>insertor: 待插入的数据 | cmd=insert&name=foo.bar&insertor={"age":12,"name":"hello"} |
| 说明     |                                          |                                                           |
| 响应头   | 同通用响应头                             |                                                           |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |                                          |                                                           |

##查询数据##

|          | 说明                                            | 例子                                                                                  |
|----------|-------------------------------------------------|---------------------------------------------------------------------------------------|
| 请求头   | 同通用请求头                                    |                                                                                       |
| 请求内容 | cmd: query<br>name: 集合的全称（集合空间.集合）<br>sort: 待排序字段名（可选参数，可不填）<br>selector: 查询结果列（可选参数，可不填）<br>skip: 跳过多少行（可选参数，可不填）<br>returnnum: 最大返回条数（可选参数，可不填）<br>filter: 查询条件（可选参数，可不填） | cmd=query&name=foo.bar&sort={"name":1}&skip=0&returnnum=1&filter={"name":"hello"} |
| 说明     |                                                 |                                                                                       |
| 响应头   | 同通用响应头                                    |                                                                                       |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回表里的记录<br>}<br>... | [{ "errno": 0 },{ "_id":{ "$oid":"54def72f0d8737161d9d6934" },"age":12,"name":"hello" }] |
| 说明     |                                                 |                                                                                       |

##查询更新数据##

|          | 说明                                             | 例子                                                                                            |
|----------|--------------------------------------------------|-------------------------------------------------------------------------------------------------|
| 请求头   | 同通用请求头                                     |                                                                                                 |
| 请求内容 | cmd: queryandupdate<br>name: 集合的全称（集合空间.集合）<br>updator: 更新操作<br>sort: 待排序字段名（可选参数，可不填）<br>selector: 查询结果列（可选参数，可不填）<br>skip: 跳过多少行（可选参数，可不填）<br>returnnum: 最大返回条数（可选参数，可不填）<br>filter: 查询条件（可选参数，可不填）<br>returnnew: 是否返回更新后记录（可选参数，可不填）<br>flag: 标志位（可选参数，可不填） | cmd=queryandupdate&name=foo.bar&updator={$set:{"age":100}}&filter={"name":"hello"}&returnnew=true |
| 说明     |  |                                                                                                 |
| 响应头   | 同通用响应头                                     |                                                                                                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回表里的记录<br>}<br>... | [{ "errno": 0 },{ "_id":{ "$oid":"54def72f0d8737161d9d6934" },"age":100,"name":"hello" }] |
| 说明     |                                                  |                                                                                                 |

> **Note:**
>
> flag既支持字符串形式，也支持数值型。数值型包括十六进制（0x开头）、八进制（0开头）、十进制。  
> 取值如下：  
> SDB_QUERY_FORCE_HINT(0x00000080)  
> SDB_QUERY_PARALLED(0x00000100)  
> SDB_QUERY_WITH_RETURNDATA(0x00000200)  
> SDB_QUERY_KEEP_SHARDINGKEY_IN_UPDATE(0x00008000)  
> 多个flag用“|”分隔，如flag=SDB_QUERY_FORCE_HINT|SDB_QUERY_PARALLED。

##查询删除数据##

|          | 说明                                            | 例子                                                                                  |
|----------|-------------------------------------------------|---------------------------------------------------------------------------------------|
| 请求头   | 同通用请求头                                    |                                                                                       |
| 请求内容 | cmd: queryandremove<br>name: 集合的全称（集合空间.集合）<br>sort: 待排序字段名（可选参数，可不填）<br>selector: 查询结果列（可选参数，可不填）<br>skip: 跳过多少行（可选参数，可不填）<br>returnnum: 最大返回条数（可选参数，可不填）<br>filter: 查询条件（可选参数，可不填） | cmd=queryandremove&name=foo.bar&filter={"name":"hello"} |
| 说明     |                                                 |                                                                                       |
| 响应头   | 同通用响应头                                    |                                                                                       |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回表里的记录<br>}<br>... | [{ "errno": 0 },{ "_id":{ "$oid":"54def72f0d8737161d9d6934" },"age":12,"name":"hello" }] |
| 说明     |                                                 |                                                                                       |

##删除记录##

|          | 说明                                     | 例子                                           |
|----------|------------------------------------------|------------------------------------------------|
| 请求头   | 同通用请求头                             |                                                |
| 请求内容 | cmd: delete<br>name: 集合的全称（集合空间.集合）<br>deletor: 删除条件 | cmd=delete&name=foo.bar&deletor={"name":"hello"} |
| 说明     |                                          |                                                |
| 响应头   | 同通用响应头                             |                                                |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |                                          |                                                |

##更新记录##

|          | 说明                                     | 例子                                                                     |
|----------|------------------------------------------|--------------------------------------------------------------------------|
| 请求头   | 同通用请求头                             |                                                                          |
| 请求内容 | cmd: update<br>name: 集合的全称（集合空间.集合）<br>updator: 更新操作<br>filter: 更新条件<br>flag: 标志位（可选参数，可不填） | cmd=update&name=foo.bar&updator={$set:{"age":100}}&filter={"name":"hello"}&flag=SDB_UPDATE_KEEP_SHARDINGKEY |
| 说明     |                                          |                                                                          |
| 响应头   | 同通用响应头                             |                                                                          |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |                                          |                                                                          |

> **Note:**  
> flag既支持字符串形式，也支持数值型。数值型包括十六进制（0x开头）、八进制（0开头）、十进制。  
> 取值如下：  
> SDB_UPDATE_KEEP_SHARDINGKEY(0x00008000)

##更新或插入记录##

|          | 说明                                     | 例子                                                                                                |
|----------|------------------------------------------|-----------------------------------------------------------------------------------------------------|
| 请求头   | 同通用请求头                             |                                                                                                     |
| 请求内容 | cmd: upsert<br>name: 集合的全称（集合空间.集合）<br>updator: 更新操作<br>filter: 更新条件（可选参数，可不填）<br> setoninsert: 插入数据（可选参数，可不填）<br>flag: 标志位（可选参数，可不填） | cmd=upsert&name=foo.bar&updator={$set:{"age":100}}&filter={"name":"hello"}&setoninsert={"sex":"male"}&flag=SDB_UPDATE_KEEP_SHARDINGKEY |
| 说明     |                                          |                                                                                                      |
| 响应头   | 同通用响应头                             |                                                                                                     |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |                                          |                                                                                                     |

> **Note:**  
> flag既支持字符串形式，也支持数值型。数值型包括十六进制（0x开头）、八进制（0开头）、十进制。  
> 取值如下：  
> SDB_UPDATE_KEEP_SHARDINGKEY(0x00008000)

##获取记录数##

|          | 说明                                     | 例子                        |
|----------|------------------------------------------|-----------------------------|
| 请求头   | 同通用请求头                             |                             |
| 请求内容 | cmd: get count<br>name: 集合的全称（集合空间.集合）<br>filter: 过滤条件（可选） | cmd=get count&name=foo.bar |
| 说明     |                                          |                             |
| 响应头   | 同通用响应头                             |                             |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>Total: 总计数<br>} | [{ "errno": 0 },{ "Total":1 }] |
| 说明     |                                          |                             |

##修改表属性##

|          | 说明                                     | 例子                                                                              |
|----------|------------------------------------------|-----------------------------------------------------------------------------------|
| 请求头   | 同通用请求头                             |                                                                                   |
| 请求内容 | cmd: alter collection<br>name: 集合的全称（集合空间.集合）<br>options: 属性 | cmd=alter collection&name=foo.bar&options={ShardingKey:{age:1},ShardingType:"hash"} |
| 说明     |                                          |                                                                                   |
| 响应头   | 同通用响应头                             |                                                                                   |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |                                          |                                                                                   |

##创建自增字段##

|     | 说明 | 例子 |
|-----|------|------|
| 请求头   | 同通用请求头 |        |
| 请求内容 | cmd: create autoincrement<br>name: 集合的全称（集合空间.集合）<br>options: 属性 | 1. 创建一个自增字段：cmd=create autoincrement&name=foo.bar&options={AutoIncrement:{Field:"id"}}<br>2. 创建多个自增字段：cmd=create autoincrement&name=foo.bar&options={AutoIncrement:[{Field:"id"},{Field:"times"}]} |
| 说明     |              |        |
| 响应头   | 同通用响应头 |        |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |              |        |

##删除自增字段##

|     | 说明 | 例子 |
|-----|------|------|
| 请求头   | 同通用请求头 |        |
| 请求内容 | cmd: drop autoincrement<br>name: 集合的全称（集合空间.集合）<br>options: 属性 | 1. 删除一个自增字段：cmd=drop autoincrement&name=foo.bar&options={Field:"id"}<br>2. 删除多个自增字段：cmd=drop autoincrement&name=foo.bar&options={Field:["id","times"]} |
| 说明     |              |        |
| 响应头   | 同通用响应头 |        |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |              |        |

##表分区##

|          | 说明                                     | 例子                                                       |  
|----------|------------------------------------------|------------------------------------------------------------|
| 请求头   | 同通用请求头                             |                                                            |
| 请求内容 | cmd: split<br>name: 集合的全称（集合空间.集合）<br>source: 源数据组<br>target: 目标数据组<br>splitpercent: 百分比<br>splitquery: 开始条件<br>splitendquery: 结束条件 | cmd=split&name=foo.bar&source=group1&target=group2&splitpercent=50 |
| 说明     |                                          |                                                            |
| 响应头   | 同通用响应头                             |                                                            |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |                                          |                                                            |

##列出数据组##

|          | 说明                                      | 例子            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd: list groups                          | cmd=list groups |
| 说明     |                                           |                 |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回数据组的内容<br>} | [{ "errno": 0 }] |
| 说明     |                                           |                 |

##收集统计信息##

|          | 说明                                      | 例子            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd: analyze<br>options: 设定分析模式、指定集合空间以及命令位置参数 | cmd=analyze<br>cmd=analyze&options={Collection:"foo.bar"} |
| 说明     | 请参考 [db.analyze\(\)](reference/Sequoiadb_command/Sdb/analyze.md) |                 |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |                                           |                 |

##查询快照##

|          | 说明                                            | 例子                                                                                  |
|----------|-------------------------------------------------|---------------------------------------------------------------------------------------|
| 请求头   | 同通用请求头                                    |                                                                                       |
| 请求内容 | cmd: snapshot [type]<br>sort: 待排序字段名（可选参数，可不填）<br>selector: 查询结果列（可选参数，可不填）<br>filter: 查询条件（可选参数，可不填）<br>hint: 快照参数，格式: { $options: {\<options\>} }（可选参数，可不填）<br>skip: 跳过多少行（可选参数，可不填）<br>returnnum: 最大返回条数（可选参数，可不填） | cmd=snapshot health&filter={"IsPrimary":false}&selector={"NodeName":null}&sort={"NodeName":-1} |
| 说明     | 请参考 [db.snapshot\(\)](reference/Sequoiadb_command/Sdb/snapshot.md) |                                                                                       |
| 响应头   | 同通用响应头                                    |                                                                                       |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>}<br>{<br>返回快照<br>}<br>... | [{ "errno": 0 },{ "NodeName": "ubuntu-test-03:41000" },{ "NodeName": "ubuntu-test-03:40000" }] |
| 说明     |                                                 |       

> **Note:**  
> [type]指快照类型，分别有：  
> contexts  
> contexts current  
> sessions  
> sessions current  
> collections  
> collectionspaces  
> database  
> system  
> catalog  
> accessplans  
> health  
> configs  

##更新配置参数##

|          | 说明                                      | 例子            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd: update config<br>configs: 配置参数，包含配置名和配置值<br>options: 命令位置参数 | cmd=update config<br>cmd=update config&configs={'diagnum':27}&options={'svcname':'20000'} |
| 说明     | 请参考 [db.updateConf\(\)](reference/Sequoiadb_command/Sdb/updateConf.md) |                 |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |

##删除配置参数##

|          | 说明                                      | 例子            |
|----------|-------------------------------------------|-----------------|
| 请求头   | 同通用请求头                              |                 |
| 请求内容 | cmd: delete config<br>configs: 配置参数，包含配置名和配置值<br>options: 命令位置参数 | cmd=update config<br>cmd=update config&configs={'diagnum':1}&options={'svcname':'20000'} |
| 说明     | 请参考 [db.deleteConf\(\)](reference/Sequoiadb_command/Sdb/deleteConf.md) |                 |
| 响应头   | 同通用响应头                              |                 |
| 响应内容 | {<br>errno: 返回值，0表示成功，其他为失败<br>description: 失败时的错误描述<br>} | [{ "errno": 0 }] |
| 说明     |
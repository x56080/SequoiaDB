[^_^]:
    安全管理工具

sdbsecuretool 是 SequoiaDB 巨杉数据库的数据安全处理工具，用于解密日志文件中的加密数据。

在 SequoiaDB v3.4.5/3.6.1/5.0.4 及以上版本中，当用户对数据进行操作后，数据库将对[节点诊断日志][diaglog]中所涉及的业务数据进行加密，以保证数据安全性。因此，用户将 SequoiaDB 升级至 v3.4.5/3.6.1/5.0.4 及以上版本后，需手动执行该工具对日志中的信息进行解密。

##语法规则##

```lang-text
sdbsecuretool [ options ] ...
```

##参数说明##

- **--help, -h**  

    获取帮助信息

- **--version, -v**  

    获取版本信息

- **--decrypt, -d \<data\>**  

    指定要解密的数据

##常见场景##

1. 进行数据操作

    以在唯一索引上插入重复键为例，执行如下语句创建唯一索引并插入数据：

    ```lang-javascript
    > db.sample.employee.createIndex('name',{name:1},{Unique:true})
    > db.sample.employee.insert({name:"Tom"})
    ```

    插入重复的数据，操作报错 -38

    ```lang-javascript
    > db.sample.employee.insert({name:"Tom"})
    sdb.js:661 uncaught exception: -38
    Duplicate key exist
    ```

2. 查看节点诊断日志中的信息，显示日志信息已加密

    ```lang-text
    2022-05-17-15.43.15.671885               Level:ERROR
    PID:31793                                TID:28228
    Function:_rtnInsertRecord                Line:156
    File:SequoiaDB/engine/rtn/rtnInsert.cpp
    Message:
    Failed to insert record SDBSECURE0000(hfIcY+zrCqbohfIcQO2uMJC1CJC+SqomTXCySmQcTNHyS+FmBXSmTXjmTXjcCP/wCJQeFA4zCqboCzDlkWCogV==) into collection: sample.employee, rc: -38
    ```

3.  将需要的日志信息进行解密

    ```lang-bash
    $ ./bin/sdbsecuretool -d "SDBSECURE0000(hfIcY+zrCqbohfIcQO2uMJC1CJC+SqomTXCySmQcTNHyS+FmBXSmTXjmTXjcCP/wCJQeFA4zCqboCzDlkWCogV==)"
    ```

    输出结果如下：

    ```lang-text
    { "_id": { "$oid": "628352132b4113f393357357" }, "name": "Tom" }
    ```

[^_^]:
    本文使用的所有引用及链接
[diaglog]:manual/Distributed_Engine/Maintainance/DiagLog/diaglog.md

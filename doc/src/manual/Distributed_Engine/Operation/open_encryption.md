[^_^]:
    开启数据加密

下述以名为“sample.employee”的集合为例，介绍开启数据加密功能的具体步骤。

1. 初始化密钥

    ```lang-javascript
    > db = new Sdb("localhost", 11810)
    > db.initSecurityKeys()
    ```

    >**Note:**
    >
    > 初始化密钥的详细说明可参考 [initSecurityKeys()][initSecurityKeys]。

2. 创建集合 sample.employee 并开启数据加密功能

    ```lang-javascript
    > db.sample.createCL("employee", {Encrypted: true})
    ```

    >**Note:**
    >
    > 创建集合的详细说明可参考 [createCL()][createcl]。

3. 查看数据加密功能是否开启成功

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CATALOG, {Name: "sample.employee"}, {AttributeDesc: ""})
    ```

    输出结果如下，如果字段 AttributeDesc 的取值包含 Encrypted，表示成功开启数据加密：

    ```lang-json
    {
      "AttributeDesc": "Encrypted"
    }
    ```

4. 备份密钥文件

    为防止密钥文件丢失或损坏，导致集合数据无法解密、读取，建议用户手动备份已生成的密钥文件，便于后续替换。以编目节点数据文件的存放路径 `/opt/sequoiadb/database/catalog/11800` 为例，执行如下语句：

    ```lang-bash
    $ cp -r /opt/sequoiadb/database/catalog/11800/security/ /opt/backup/
    ```

[^_^]:
     本文使用的所有引用及链接
[createcl]:manual/Manual/Sequoiadb_Command/SdbCS/createCL.md
[initSecurityKeys]:manual/Manual/Sequoiadb_Command/Sdb/initSecurityKeys.md

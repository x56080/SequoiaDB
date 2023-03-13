
下述以名为“s1”的 Schema 为例，介绍 Schema 的相关操作。

##使用##

1. 开启集合 sample.employee 的 [Information Schema][infoschema] 功能

    ```lang-javascript
    > db.sample.employee.alter({EnableInfoSchema: true})
    ```

2. 创建名为“s1”的 Schema

    ```lang-javascript
    > db.createSchema("s1", {"a": {Type: "int32"}})
    ```

    > **Note:**
    >
    > 创建 Schema 的详细参数说明可参考 [createSchema()][createSchema]。

3. 查看 Schema 的相关信息

    ```lang-javascriptt
    > db.list(SDB_LIST_SCHEMAS)
    ```

    输出结果如下：

    ```lang-json
    {
      "_id": {
        "$oid": "63eaf867bc95fa7ff3ce38e5"
      },
      "Name": "s1",
      "Version": 0,
      "StrictMode": false,
      "Columns": {
        "a": {
          "Type": "int32",
          "Restrict": 0,
          "RestrictDesc": ""
        }
      }
    }
    ```

4. 将集合 sample.employee 与 Schema 进行绑定

    ```lang-javascript
    > db.sample.employee.addSchema("s1")
    ```

5. 查看是否绑定成功

    ```lang-javascript
    > db.snapshot(SDB_SNAP_CATALOG, {Schema: {$isnull:0}}, {Name:"", AttributeDesc:"", Schema:""})
    ```

    输出结果如下，如果存在字段值 EnableInfoSchema 和字段 Schema，表示该集合已具备 Information Schema 特性：

    ```lang-text
    {
      "AttributeDesc": "Compressed | EnableInfoSchema",
      "Name": "sample.employee",
      "Schema": "s3"
    }
    ```

##参考##

更多操作可参考

| 操作                                               | 说明                   |
|----------------------------------------------------|------------------------|
| [db.getSchema()][getSchema]                        | 获取指定 Schema 的引用 | 
| [Schema.addColumn()][addColumn]                    | 新增 Schema 的字段     |
| [Schema.alterColumn()][alterColumn]                | 修改 Schema 的字段定义 |
| [Schema.renameColumn()][renameColumn]              | 重命名 Schema 中的字段 |
| [Schema.dropColumn()][dropColumn]                  | 删除 Schema 中的字段   |
| [db.dropSchema()][Sdb.dropSchema]                  | 删除指定的 Schema      |

[^_^]:
    本文使用的所有引用和链接
[infoschema]:manual/Distributed_Engine/Architecture/infoSchema.md
[createSchema]:manual/Manual/Sequoiadb_Command/Sdb/createSchema.md
[SDB_LIST_SCHEMAS]:manual/Manual/List/SDB_LIST_SCHEMAS.md
[SDB_SNAP_CATALOG]:manual/Manual/Snapshot/SDB_SNAP_CATALOG.md
[getSchema]:manual/Manual/Sequoiadb_Command/Sdb/getSchema.md
[addColumn]:manual/Manual/Sequoiadb_Command/SdbSchema/addColumn.md
[alterColumn]:manual/Manual/Sequoiadb_Command/SdbSchema/alterColumn.md
[renameColumn]:manual/Manual/Sequoiadb_Command/SdbSchema/renameColumn.md
[dropColumn]:manual/Manual/Sequoiadb_Command/SdbSchema/dropColumn.md
[Sdb.dropSchema]:manual/Manual/Sequoiadb_Command/Sdb/dropSchema.md


##概念##

大对象（LOB）功能旨在突破 SequoiaDB 的单条记录最大长度为 16MB 的限制，为用户写入和读取更大型记录提供便利。LOB 记录的大小目前不受限制。

每一个 LOB 记录拥有一个 OID，通过指定集合及 OID 可以访问一条 LOB 记录。在非分区集合及哈希分区集合中均可使用 LOB 功能。集合间不共享 LOB 记录。当一个集合被删除时，其拥有的 LOB 记录自动删除。

LOB 记录的存储格式：

![LOB的记录格式](data_model/lob.jpg)

每个 LOB 记录包含若干个分片。分片所占空间大小均为 LobPageSize（创建集合空间时指定，默认为 256 KB，请参考 [Sdb.createCS\(\)](reference/Sequoiadb_command/Sdb/createCS.md)）。在哈希分区中，LOB 记录的每一个分片会被按照 OID 加分片序号分散存储在相应的分区组中。其哈希空间与所属集合的哈希空间相同。

目前 LOB 的存储格式为二进制类型。

##支持的操作##

| 操作 | 参见 | 备注 |
| ---- | ---- | ---- |
| 创建 | [SdbCollection.putLob()](reference/Sequoiadb_command/SdbCollection/putLob.md) | 向集合中创建一个 LOB 记录。LOB 记录一旦创建完毕，其内容无法再做更改。 |
| 读取 | [SdbCollection.getLob()](reference/Sequoiadb_command/SdbCollection/getLob.md) | 从集合中读取某个 LOB 记录。另外，[C API](driver/c_driver/c_api.md) 中提供对 LOB 记录的 seek 操作。 |
| 删除 | [SdbCollection.deleteLob()](reference/Sequoiadb_command/SdbCollection/deleteLob.md) | 删除集合中的某个 LOB 对象。 |
| 列表 | [SdbCollection.listLobs()](reference/Sequoiadb_command/SdbCollection/listLobs.md) | 列出集合中所有 LOB 对象。 |

##示例##

在 Sdb Shell 中将本地文件 mylob 上传至集合 foo.bar 中：

```lang-javascript
> db.foo.bar.putLob( '/opt/mylob' )
```

在 Sdb Shell 中查看 LOB 记录及对应的 OID：

```lang-javascript
> db.foo.bar.listLobs()
{
  "Size": 76602,
  "Oid": {
    "$oid": "5435e7b69487faa663000897"
  },
  "CreateTime": {
    "$timestamp": "2018-02-26-12.51.43.628000"
  },
  "Available": true
}
```

在 Sdb Shell 中将集合 foo.bar 中的 OID 为 5435e7b69487faa663000897 的LOB 记录下载到本地文件 mylob 中：

```lang-javascript
> db.foo.bar.getLob( '5435e7b69487faa663000897', '/opt/mylob' )
```

在 Sdb Shell 中将集合 foo.bar 中的 OID 为 5435e7b69487faa663000897 的 LOB 记录删除：

```lang-javascript
> db.foo.bar.deleteLob( '5435e7b69487faa663000897' )
```


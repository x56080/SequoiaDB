[^_^]:
    主密钥生成工具

sdbmkgen 是 SequoiaDB 巨杉数据库的主密钥生成工具，用于生成新的主密钥。

##语法规则##

**sdbmkgen <--file | -f arg>**

##参数说明##

| 参数名    | 缩写 | 描述 | 是否必填 |
| --------- | ---- | ---- | -------- |
| --help    | -h | 获取帮助信息 | 否 |
| --version | -v | 获取版本信息 | 否 |
| --file    | -f | 指定存储主密钥的文件名称<br>如果指定的文件已存在，原文件的内容将被替换为主密钥信息 | 是 |

##常见场景##

在当前路径下生成主密钥，并指定文件名为 `sm2_key_pair.pem`

```lang-bash
$ sdbmkgen -f sm2_key_pair.pem
```

[^_^]:
     本文使用的所有引用及链接
[encryption]:manual/Distributed_Engine/Architecture/data_encryption.md
[snapshot]:manual/Manual/Snapshot/SDB_SNAP_CATALOG.md
[init]:manual/Manual/Sequoiadb_Command/Sdb/initSecurityKeys.md
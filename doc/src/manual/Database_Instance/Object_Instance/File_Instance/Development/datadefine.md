当用户将 Linux 下创建的 `/home/sdbadmin/guestdir/` 目录成功挂载到 SequoiaDB 巨杉数据库后，仍然可以在 `guestdir` 目录下执行常见的创建子目录、创建文件、修改文件、删除文件等命令，也可以通过常见的文件 API 接口对目录文件进行操作，此时所有的文件内容及目录结构都存储于 SequoiaDB 中。
  
##挂载目录的信息查看##

通过 SequoiaFS 将 `/home/sdbadmin/guestdir/` 目录挂载到 SequoiaDB 中，挂载的目标集合为 mountcs.mountcl 

```lang-javascript
> var db = new Sdb("localhost", 11810) 
> db.list(SDB_LIST_COLLECTIONS)
```

挂载成功后的挂载目录和如下五个集合相关：

```lang-text
{
  "Name": "mountcs.mountcl"
}
{
  "Name": "mountcs.mountcl_FS_SYS_DirMeta"
}
{
  "Name": "mountcs.mountcl_FS_SYS_FileMeta"
}
{
  "Name": "sequoiafs.maphistory"
}
{
  "Name": "sequoiafs.sequenceid"
}
```

- mountcs.mountcl：挂载集合，以 Lob 形式存储挂载目录下的文件内容；
- mountcs.mountcl_FS_SYS_DirMeta：存储挂载目录及子目录的元数据；
- mountcs.mountcl_FS_SYS_FileMeta：存储挂载目录及子目录中文件的元数据；
- sequoiafs.sequenceid：目录元数据中所有目录记录的 id 序列表，用于构造目录的唯一性，记录目录之间的所属关系；
- sequoiafs.maphistory：挂载历史信息集合，记录历史挂载的关键数据信息，每次挂载都会产生一条挂载数据。 

###示例###

查询挂载历史信息集合

```lang-javascript
>  db.sequoiafs.maphistory.find()
```

得到挂载后产生的挂载信息

```lang-text
{
  "_id": {
    "$oid": "5ec5af7ae27d726c75b90b2f"
  },
  "SourceCL": "mountcs.mountcl",
  "DirMetaCL": "mountcs.mountcl_FS_SYS_DirMeta",
  "FileMetaCL": "mountcs.mountcl_FS_SYS_FileMeta",
  "Address": "ens160:192.168.20.69;",
  "MountPoint": "/home/sdbadmin/guestdir/",
  "MountTime": {
    "MountTime": "2020-05-21-06.30.18.793845"
  }
}
```

挂载历史信息中字段含义如下：    

|参数名     |类型   |描述                      |
|-----------|-------|--------------------------|
|SourceCL   |string | 目标集合名称             |
|DirMetaCL  |string | 目录元数据集合名称       |
|FileMetaCL |string | 文件元数据集合名称       |
|Address    |string | 挂载目录所在的主机地址   |
|MountPoint |string | 挂载目录的全路径         |

##挂载目录下文件和子目录信息查看##

1. 在挂载目录下创建文件 `testfile` 并写入"hello, this is a testfile!" 

   ```lang-bash
   $ cd /home/sdbadmin/guestdir/
   $ touch testfile
   $ echo 'hello, this is a testfile!' >> testfile
   ```

2. 创建子目录 `testdir`

   ```lang-bash
   $ mkdir testdir
   ```

3. 查询目录元数据集合

   ```lang-javascript
   > db.mountcs.mountcl_FS_SYS_DirMeta.find()
   ```
   得到根目录和子目录 `testdir` 的元数据信息

   ```lang-text
   {
     "AccessTime": 1589966920958,
     "CreateTime": 1589966920958,
     "Gid": 0,
     "Id": 1,
     "Mode": 16877,
     "ModifyTime": 1589966920958,
     "NLink": 3,
     "Name": "/",
     "Pid": 0,
     "Size": 4096,
     "SymLink": "",
     "Uid": 0,
     "_id": {
       "$oid": "5ec4f848385b7a63e66391ba"
     }
   }
   {
     "_id": {
       "$oid": "5ec5b26ee27d726c75b90b31"
     },
     "Name": "testdir",
     "Mode": 16877,
     "Uid": 501,
     "Gid": 501,
     "Pid": 1,
     "Id": 2,
     "NLink": 2,
     "Size": 4096,
     "CreateTime": 1590014574966,
     "ModifyTime": 1590014574966,
     "AccessTime": 1590014574966,
     "SymLink": ""
   }
   Return 2 row(s).
   ```

   目录元数据信息中的字段含义如下：

   |参数名     |类型      |  描述                    |
   |-----------|--------  |--------------------------|
   |_id        | OID      | 对象 ID                  |
   |Name       | string   | 目录名称                 |
   |Mode       | number   | 目录属性模式             |
   |Uid        | number   | 目录属主 ID              |
   |Gid        | number   | 目录组属主 ID            |
   |Pid        | number   | 目录父目录 ID，不同于_id |
   |Id         | number   | 目录 ID                  |
   |NLink      | number   | 目录 link                |
   |Size       | number   | 目录大小                 |
   |CreateTime | number   | 创建时间                 |
   |ModifyTime | number   | 修改时间                 |
   |AccessTime | number   | 访问时间                 |
   |SymLink    | string   | 软链接                   |

4. 查询文件元数据集合 

   ```lang-javascript
   > db.mountcs.mountcl_FS_SYS_FileMeta.find()
   ```

   得到 `testfile` 文件元数据信息

   ```lang-text
   {
     "AccessTime": 1590044385674,
     "CreateTime": 1590014562000,
     "Gid": 501,
     "LobOid": "00005ec697603500046e0c44",
     "Mode": 33188,
     "ModifyTime": 1590044385674,
     "NLink": 1,
     "Name": "testfile",
     "Pid": 1,
     "Size": 27,
     "SymLink": "",
     "Uid": 501,
     "_id": {
       "$oid": "5ec5b262e27d726c75b90b30"
     }
   }
   Return 1 row(s).
   ```

   文件元数据信息中的字段含义如下：
   
   |参数名     |类型  | 描述                    |
   |-----------|------|-------------------------|
   |_id        | OID  | 对象 ID                 |
   |Name       |string| 文件名称                |
   |Mode       |number| 文件属性模式            |
   |Uid        |number| 文件属主 ID             |
   |Gid        |number| 文件组属主 ID           |
   |Pid        |number| 文件父目录 ID，不同于_id|
   |LobOid     |string| 文件对应 lob 对象 ID    |
   |NLink      |number| 文件 link 数            |
   |Size       |number| 文件大小                |
   |CreateTime |number| 创建时间                |
   |ModifyTime |number| 修改时间                |
   |AccessTime |number| 访问时间                |
   |SymLink    |string| 软链接                  |

5. 查看挂载集合

   ```lang-javascript
   > db.mountcs.mountcl.listLobs()
   ```

   输出结果中可以看到有一条 Lob 数据

   ```lang-text
   {
     "Size": 27,
     "Oid": {
       "$oid": "00005ec697603500046e0c44"
     },
     "CreateTime": {
       "$timestamp": "2020-05-21-14.59.44.776000"
     },
     "ModificationTime": {
       "$timestamp": "2020-05-21-14.59.51.729000"
     },
     "Available": true,
     "HasPiecesInfo": false
   }
   Return 1 row(s).
   ```



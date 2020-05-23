本章介绍挂载目录后数据库元数据的存放规则。

将 Linux 下创建的 /opt/sequoiadb/guestdir/ 目录挂载到 SequoiaDB 中，挂载之后 guestdir 目录的操作仍然和普通文件系统目录一样，可以在 guestdir 目录下执行常见的创建子目录、创建文件、修改文件、删除文件等命令，也可以通过常见的文件 API 接口对目录文件进行操作，此时所有的文件内容及目录结构都存储于 SequoiaDB 中。

##挂载目录的信息查看##

通过 SequoiaFS 将 /opt/sequoiadb/guestdir/ 目录挂载到 SequoiaDB 中，挂载的目标集合为 mountcs.mountcl，挂载成功后，挂载目录和如下五个集合相关：

```lang-javascript
> var db = new Sdb("localhost", 11810) 
```
```lang-javascript
> db.list(SDB_LIST_COLLECTIONS)
```
```
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

- mountcs.mountcl 挂载集合，存储挂载目录下的文件内容，以 Lob 形式存储。
- mountcs.mountcl_FS_SYS_DirMeta 存储挂载目录及子目录的元数据。
- mountcs.mountcl_FS_SYS_FileMeta 存储挂载目录及子目录中文件的元数据。
- sequoiafs.sequenceid 目录元数据中目录记录的id序列表，用于构造目录的唯一性，记录目录之间的所属关系。 
- sequoiafs.maphistory 为挂载历史信息表，记录历史挂载的关键数据信息，每次挂载都会产生一条挂载数据。  

查询挂载历史信息表。

```lang-javascript
>  db.sequoiafs.maphistory.find()
```

可以挂载后产生的挂载信息。

```
{
  "_id": {
    "$oid": "5ec5af7ae27d726c75b90b2f"
  },
  "SourceCL": "mountcs.mountcl",
  "DirMetaCL": "mountcs.mountcl_FS_SYS_DirMeta",
  "FileMetaCL": "mountcs.mountcl_FS_SYS_FileMeta",
  "Address": "ens160:192.168.20.69;",
  "MountPoint": "/opt/sequoiadb/guestdir/",
  "MountTime": {
    "MountTime": "2020-05-21-06.30.18.793845"
  }
}
```
其基本含义如下：    

|记录名称   | 描述说明                 |
|-----------|--------------------------|
|SourceCL   | 目标映射集合名称         |
|DirMetaCL  | 目录元数据集合名称       |
|FileMetaCL | 文件元数据集合名称       |
|Address    | 挂载目录所在的主机地址   |
|MountPoint | 挂载目录的全路径         |

##挂载目录下文件和子目录信息查看##

在本示例中，在挂载目录下创建了文件 testfile 并写入'hello, this is a testfile!'，创建了子目录 testdir。

```lang-bash
$ cd /opt/sequoiadb/guestdir/
$ touch testfile
$ echo 'hello, this is a testfile!' >> testfile
$ mkdir testdir
```

查询目录元数据集合。

```lang-javascript
> db.mountcs.mountcl_FS_SYS_DirMeta.find()
```

可以查到根目录和 testdir 这个子目录的元数据信息。

```
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
Takes 0.003649s.
```

目录元数据信息的具体含义如下：

|记录名称   | 描述说明                 |数据类型|
|-----------|--------------------------|--------|
|_id        | 对象ID                   | OID  |
|Name       | 目录名称                 |字符串|
|Mode       | 目录属性模式             |整数|
|Uid        | 目录属主                 |整数|
|Gid        | 目录组属主               |整数|
|Pid        | 目录父目录ID，不同于_id  |长整数|
|Id         | 目录ID                   |长整数|
|NLink      | 目录link                 |整数|
|Size       | 目录大小                 |长整数|
|CreateTime | 创建时间                 |长整数|
|ModifyTime | 修改时间                 |长整数|
|AccessTime | 访问时间                 |长整数|
|SymLink    | 软链接                  |字符串|

查询文件元数据集合。  

```lang-javascript
> db.mountcs.mountcl_FS_SYS_FileMeta.find()
```

可以查到 testfile 文件元数据信息记录。

```
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
Takes 0.002549s.
```

文件元数据信息具体含义如下：

|记录名称   | 描述说明                |数据类型|
|-----------|-------------------------|------|
|_id        | 对象ID                  | OID  |
|Name       | 文件名称                |字符串|
|Mode       | 文件属性模式            |整数  |
|Uid        | 文件属主                |整数  |
|Gid        | 文件组属主              |整数  |
|Pid        | 文件父目录ID，不同于_id |长整数|
|LobOid     | 文件对应lob对象ID       |字符串|
|NLink      | 文件link数              |整数  |
|Size       | 文件大小                |长整数|
|CreateTime | 创建时间                |长整数|
|ModifyTime | 修改时间                |长整数|
|AccessTime | 访问时间                |长整数|
|SymLink    | 软链接                  |字符串|

查看挂载集合。

```lang-javascript
> db.mountcs.mountcl.listLobs()
```

可以看到有一条 Lob 数据。

```
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
Takes 0.135840s.
```



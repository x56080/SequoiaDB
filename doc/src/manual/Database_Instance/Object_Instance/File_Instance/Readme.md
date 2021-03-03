SequoiaFS 文件系统是 SequoiaDB 巨杉数据库基于 FUSE 在 Linux 系统下实现的一套文件系统，支持通用的文件操作 API。

用户可以通过 SequoiaFS 将本地目录挂载到 SequoiaDB 的目标集合，在挂载目录下可以使用通用文件系统 API 对文件和目录进行操作。SequoiaFS 使用 SequoiaDB 的元数据集合存储文件和目录的属性，使用大对象（LOB）存储文件的内容，实现类似 NFS 的分布式网络文件系统。
  
![系统架构图][model]
     
SequoiaFS支持的fuselib库版本
----
- FUSE library v2.8.6 及以上版本
- fusermount v2.8.6 及以上版本  


[^_^]:
     本文使用的所有引用和链接
[model]:images/Database_Instance/Object_Instance/File_Instance/model.png
SequoiaFS文件系统是基于FUSE在Linux系统下实现的一套文件系统，支持通用的文件操作API。SequoiaFS利用SequoiaDB的元数据集合存储文件和目录的属性信息，lob对象存储文件的数据内容，从而实现了类似NFS分布式网络文件系统。用户可以将远程SequoiaDB的某个目标集合通过映射的方式挂载到本地FS节点，在FS节点的挂载目录下实现通过通用文件系统API对文件和目录进行操作。

下面为其基本逻辑结构图： 
  
![](sequoiafs/model.png)
     
##SequoiaFS支持的fuselib库版本##
FUSE library version: 2.8.6及以上版本  
fusermount version: 2.8.6及以上版本  
  
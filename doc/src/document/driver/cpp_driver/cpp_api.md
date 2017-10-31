此部分是相关 [C++ API](api/cpp/html/index.html) 文档。

##历史更新情况：##

**Version 2.6**

（1）sdb 类新添加的接口：getLastAliveTime，获取sdb最后与服务器端交互时间到标准计时点的秒数


**Version 1.10**

（1）SdbCollection 类添加的接口：

```
explain，获取查询的访问计划
createLob，创建一个新的lob
openLob，打开一个已存在的lob，该版本中，打开的lob只用于读操作
removeLob，删除一个lob
listLobs，列出当前collection中的所有lob

```

（2）添加类 sdbLob 用于大对象操作，其接口如下：

```
read，从lob中读取数据
write，把数据写入lob中
seek，设置读起始位置，该版本中，seek只用于读操作
close，关闭一个新创建的或打开的lob
getOid，获取lob的oid
getSize，获取lob的大小
getCreateTime，获取lob的创建时间
```

**Version 1.8**

（1）sdb 类新添加的接口：

```
connect，可提供多个地址，接口随机选择一个有效的地址连接。
createCollectionSpace，提供一个 BSONObject 的选项，使创建集合空间更加灵活
backupOffline，离线备份支持更多的选项
createDomain，创建域
getDomain，获取域
dropDomain，删除域
listDomain，列出所有域
```

（2）sdbCollection 类新添加的接口：

```
alterCollection，修改集合（表）属性
```

（3）添加 Domain 类用于与域相关的操作

**Version 1.6**

（1）添加类 Node 来取代原来的类 ReplicaNode。类 ReplicaNode 以及与它相关的方法将在 version 2.x 中被弃用。


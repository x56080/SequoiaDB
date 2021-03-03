此部分是相关 [C API](api/c/html/index.html) 文档。

##历史更新情况：##

**Version 1.10**

（1） 添加获取查询访问计划的接口：

```
sdbExplain，获取查询的访问计划
```
（2） 添加用于大对象（lob）操作的接口：

```
sdbListLobs，列出集合中的所有lob
sdbOpenLob，创建或打开一个lob
sdbCloseLob，关闭一个lob
sdbRemoveLob， 删除一个lob
sdbSeekLob，设置读起始位置，该版本中，seek只用于读操作
sdbReadLob，从lob中读取数据
sdbWriteLob，把数据写入lob
sdbGetLobSize，获取lob的大小
sdbGetLobCreateTime，获取lob的创建时间
```
**Version 1.8**

新添加接口：

```
sdbConnect1，可提供多个地址，接口随机选择一个有效的地址连接
sdbCreateCollectionSpaceV2，提供一个 bson 的选项，使创建集合空间更加灵活
sdbAlterCollection，修改集合（表）属性
sdbCreateDomain，创建域
sdbDropDomain，删除域
sdbGetDomain，获取域句柄
sdbListDomains，列出所有域
sdbReleaseDomain，删除域句柄
sdbAlterDomain，更改域属性
```

**Version 1.6**

（1）使用 sdbNodeHandle 来取代原来的 sdbReplicaNodeHandle。sdbReplicaNodeHandle 将在 version 2.x 中被弃用。

（2）使用概念“node”取代原来的“replica node”，和“replica node”相关的 API 接口将保留，直到 version 2.x 会被弃用。

更多详情可查看辅助API [BASE64C API](api/bson/html/base64c_8h.html) 和 [JSTOBSON API](api/bson/html/jstobs_8h.html)。

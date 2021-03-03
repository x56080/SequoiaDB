##语法##
***rg.getSlave()***

获取当前分区组的从节点。

##返回值##

返回分区组的从节点，类型为 Object 对象；如果分区组内存在多个从节点，则随机返回其中一个从节点。

##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

获取 group1 分区组的从节点

```lang-javascript
> var rg = db.getRG("group1")
> rg.getSlave()
hostname1:11830
```

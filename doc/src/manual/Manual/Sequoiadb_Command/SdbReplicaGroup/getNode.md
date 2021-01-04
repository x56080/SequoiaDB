
##语法##
***rg.getNode( \<nodename|hostname\>, \<servicename\> )***

获取当前复制组的指定节点。

##参数描述##

| 参数名 | 参数类型 | 描述   | 是否必填 |
| ------ | -------- | ------ | -------- |
| nodename | string | 节点名称。| nodename 与 hostname 二选一 |
| hostname | string | 主机名。  | hostname 与 nodename 二选一 |
| servicename | string | 服务器名称。 | 是 |

> **Note:**  
> rg.getNode() 方法定义了两个参数，第一个参数可是节点名称也可以是主机名，第二个参数为服务器名称。两个参数的类型都是字符串型，且必填。  
> 格式：( "<节点名称>|<主机名>", "<服务器名称>" )

##返回值##

返回复制组内指定的节点，类型为 Object 对象。

##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

指定主机名和服务器名，获取该指定节点

```lang-javascript
> var rg = db.getRG("group1")
> rg.getNode( "hostname1", "11830" )
hostname1:11830
```

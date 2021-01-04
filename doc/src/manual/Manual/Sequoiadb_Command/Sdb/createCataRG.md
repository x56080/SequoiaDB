

##语法##
***db.createCataRG( \<host\>, \<service\>, \<dbpath\>, [config] )***

新建一个编目复制组，同时创建并启动一个编目节点。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| host | string | 指定编目节点的主机名。 | 是 |
| service | int/string | 指定编目节点的服务端口，请确保该端口号，以及往后延续的3个端口号未被占用；如设置为11800，请确保11800/11801/11802/11803端口都未被占用。 | 是 |
| dbpath | string | 数据文件路径，用于存放编目数据文件，需确保数据管理员（安装时创建，默认为 sdbadmin）用户有写权限。 | 是 |
| config | json | 参数为可选参数，用于配置更多细节参数，格式必须为 json 格式，参数参见数据库配置一节；如需要配置日志大小参数{ logfilesz: 64 }。 | 否 |

> **Note:**
>
> - 如果配置路径不以“/”开头，数据文件存放路径将是数据库管理员用户(默认为sdbadmin)的主目录(默认为/home/sequoiadb) + 配置的路径。
> - 请确保数据文件存放路径的权限，如果 SequoiaDB 采用的默认安装，那么给路径赋予 sdbadmin 权限。
> - service 目前建议直接采用port。

##返回值##

返回新建编目复制组的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。


##示例##

- 在名为：hostname1 的主机上创建一个编目节点组，服务端口为：11800，数据文件存放路径为：/opt/sequoiadb/database/cata/11800

 ```lang-javascript
 > db.createCataRG( "hostname1", 11800, "/opt/sequoiadb/database/cata/11800" )
 ```

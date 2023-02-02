
协调节点（coord）和数据节点（data）对外提供 REST 接口访问，同时支持 HTTP 和 HTTPS 协议。

##通用请求头##

| 字段           | 描述                                   | 示例                 |
| -------------- | -------------------------------------- | -------------------- |
| Content-Type   | 请求的数据类型                         | application/x-www-form-urlencoded;charset=UTF-8 |
| Content-Length | 请求的长度                             | 54                   |
| Host           | 请求的主机名，格式为 `<ip>:<httpname>`<br>httpname 表示协调节点或数据节点的 REST 服务端口，可通过[配置快照][snapshot]查看对应端口号 | 192.168.1.214:11814  |
| Accept         | 希望应答的数据类型，如果不指定该字段，默认响应 text/html （文本格式）的数据类型 | application/json |
| SdbUser | 请求主机的用户名<br>仅在请求的主机设置了鉴权时指定 | SdbUser:sdbadmin |
| SdbPasswd | 请求主机的用户密码<br>● 仅在请求的主机设置了鉴权时指定<br>● 密码需以密文形式输入，可通过集合 [SYSAUTH.SYSUSRS][sysusrs] 查看用户名对应的密码 | SdbPasswd:7fb667faf9ed4b765a4bba50beaf46d7 |

```lang-http
POST / HTTP/1.0
Content-Type: application/x-www-form-urlencoded;charset=UTF-8
Accept: application/json
Content-Length: 54
Host: 192.168.1.214:11814
SdbUser:sdbadmin
SdbPasswd:7fb667faf9ed4b765a4bba50beaf46d7
```

##通用响应头##

| 字段           | 描述           | 示例      |
| -------------- | -------------- | --------- |
| Content-Type   | 响应内容的类型，如果请求 Accept 为 application/json，<br>应答 Content-Type 为 application/json，否则应答 Content-Type 为 text/html | application/json |
| Content-Length | 响应内容的长度 | 35        |

```lang-http
HTTP/1.1 200 Ok
Content-Length: 35
Content-Type: application/json
```

[^_^]:
     本文使用的所有引用及链接
[snapshot]:manual/Manual/Snapshot/SDB_SNAP_CONFIGS.md
[sysusrs]:manual/Manual/Catalog_Table/SYSUSRS.md
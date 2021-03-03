协调节点（coord）和数据节点（data）对外提供REST接口访问，同时支持HTTP和HTTPS协议。

##通用请求头##

|                | 说明                                   | 例子 |
| -------------- | -------------------------------------- | -------------------- | 
| Content-Type   | 请求内容的类型                         | application/x-www-form-urlencoded;charset=UTF-8 |
| Content-Length | 请求内容的长度                         | 54 |
| Host           | 主机名（协调节点或数据节点的服务地址） | 192.168.1.214:11814 |        

```
POST / HTTP/1.0
Content-Type: application/x-www-form-urlencoded;charset=UTF-8
Content-Length: 54
Host: 192.168.1.214:11814
```

##通用响应头##

|                |  说明          |  例子 |
| -------------- | -------------- | --------- |
| Content-Type   | 响应内容的类型 | text/html |
| Content-Length | 响应内容的长度 | 35 |

```
HTTP/1.1 200 Ok
Content-Length: 35
Content-Type: text/html
```

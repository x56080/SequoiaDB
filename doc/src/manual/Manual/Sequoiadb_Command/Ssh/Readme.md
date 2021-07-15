Ssh 类主要用于连接主机，包含的函数如下：

| 名称 | 描述 |
|------|------|
| Ssh() | 使用 SSH 方式连接主机 |
| close() | 关闭 Ssh 连接 |
| exec() | 执行 Shell 指令 |
| getLastOut() | 获取上次命令执行的返回结果 |
| getLastRet() | 获取上次命令执行返回的错误码 |
| getLocalIP() | 获取本地 IP 地址 |
| getPeerIP() | 获取远程主机的 IP 地址 |
| pull() | 从远程主机复制文件到本地 |
| push() | 从本地主机上复制文件到远程主机上 |
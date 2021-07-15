File 类主要用于文件操作，包含的函数如下：

用户可以通过 File 类进行文件操作，该类包含的函数如下：

| 名称 | 描述 |
|------|------|
| File() | 打开文件或者创建新文件 |
| chgrp() | 设置文件的用户组 |
| chmod() | 设置文件权限 |
| chown() | 设置文件的所有者 |
| close() | 关闭文件 |
| copy() | 复制文件 |
| exist() | 判断文件是否存在 |
| find() | 查找文件 |
| getSize() | 获取文件的大小 | 
| getUmask() | 获取新建文件权限的掩码 |
| isDir() | 判断指定文件是否为目录 |
| isEmptyDir() | 判断指定目录是否为空目录 |
| isFile() | 判断指定文件是否为普通文件 |
| list() | 列出当前目录的文件 |
| md5() | 获取文件的 md5 值 |
| mkdir() | 创建目录 |
| move() | 移动文件 |
| read() | 读取文本文件 |
| readContent() | 读取二进制文件内容并存入 FileContent 对象 |
| readLine() | 读取文件的一行数据 |
| remove() | 删除文件或者目录 |
| scp() | 远程拷贝文件 |
| seek() | 移动文件游标 |
| setUmask() | 设置新建文件的权限掩码 |
| stat() | 显示文件的状态信息 |
| write() | 往文本文件中写内容 |
| writeContent() | 将 fileContent 对象中的二进制内容写入文件中 |
| truncate() | 将文件截断到给定的长度 |
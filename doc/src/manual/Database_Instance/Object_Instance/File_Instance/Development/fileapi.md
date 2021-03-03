本文档主要介绍 SequoiaFS 支持的文件操作 API 及使用示例。  

API 接口
====

SequoiaFS 现支持以下文件操作 API：

- **opendir(const char *name)**

 打开目录文件

- **readdir(DIR *dir)**

 读取目录文件

- **closedir(DIR *dir)**

 关闭目录文件

- **open(const char *pathname, int flags, [mode_t mode])**

 创建或打开一个文件

 * flags 只支持"O_RDONLY"、"O_WRONLY"、"O_RDWR"和"O_CREAT"
 * mode 缺省，则默认为 644

- **close(int fd)**

 读取文件数据

- **remove(const char *pathname)**

 删除文件

- **lseek(FILE *stream, long offset, int whence)**

 设置读写偏移

- **read(int fd, void *buf, size_t count)**

 读取文件数据

- **write(int fd, const void *buf, size_t count)**

 写文件数据

- **stat(const char *pathname, struct stat *buf)**

 获取文件的属性信息

- **utime(const char *pathname, struct utimebuf *buf)**

 更改访问和修改时间

- **link(const char *oldpath, const char *newpath)**

 创建链接文件（硬链接）

- **unlink(const char *pathname)**

 删除指定文件，如果该文件为最后的链接点，则文件会被删除；如果为符号链接，则链接删除

- **symlink(const char *oldpath, const char *newpath)**

 创建符号链接文件, oldpath 指定文件允许不存在

- **truncate(const char *pathname, off_t length)**

 截取文件内容，将 path 指定的文件大小改为参数 length 的大小；如果原来文件比 length 大，则超过的部分会被删除

- **mkdir(const char *pathname, mode_t mode)**

 创建目录文件

- **rmdir(const char *pathname)**

 删除目录文件

- **rename(const char *pathname)**

 更改文件名称

- **chmod(const char *pathname, mode_t mode)**

 更改文件权限

> **Note：** 
>  
> 关于系统命令，支持基于以上接口的一些常见系统命令，如：vi、cp、rm、touch、cat、mv、ln、chown、tar 等，超出以上接口之外的系统命令暂时不支持。

##API使用示例##

以下示例通过 API 在 `guestdir` 目录下创建了一个 `testfile` 文件，并写入 testdata 内容。  

```lang-c++
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

static char testdata[] = "abcdefghijklmnopqrstuvwxyz";
static int testdatalen = sizeof(testdata) - 1;
#define testfile "/home/sdbadmin/guestdir/testfile"

int main()
{
    int rc = 0;
    int fd = 0;
    const char *data = testdata;
    int datalen = testdatalen;

    fd = open(testfile,  O_WRONLY|O_CREAT);
    if(0 > fd)
    {
        printf("Failed to open file:%s\n", testfile);
        goto error;
    }

    rc = write(fd, data, datalen);
    if(0 > rc)
    {
        printf("Failed to write file:%s\n", testfile);
        goto error;
    }

    rc = lseek(fd, 4, SEEK_SET);
    if(0 > rc)
    {
        printf("Failed to lseek file:%s\n", testfile);
        goto error;
    }

    rc = write(fd, "DF", 2);
    if(0 > rc)
    {
        printf("Failed to write file:%s\n", testfile);
        goto error;
    }

    rc = close(fd);
    if(0 > rc)
    {
        printf("Failed to close file:%s\n", testfile);
        goto error;
    }
done:
    return rc;
error:
    goto done;
}     
```  



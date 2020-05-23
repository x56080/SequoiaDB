本章介绍 SequoiaFS 支持的文件操作 API 及使用样例。  

API 接口
====

SequoiaFS 现支持以下文件操作 API：

|接口函数   | 参数                     | 描述          
|-----------|--------------------------|-----------------------------------------------------------------------------------------------------------
|opendir()  | const char *name         | 打开目录文件                                                                                             |
|readdir()  | DIR *dir                 | 读取目录文件                                                                                             |
|closedir() | DIR *dir                 | 关闭目录文件                                                                                             |
|open()     | const char *pathname     | 创建或打开一个文件，flags 只支持O_RDONLY, O_WRONLY, O_CREATE, <br>其他报错。忽略可选参数 mode，默认权限644。 |
|           | int flags                |                                                                                                          |
|           | [mode_t mode]            |                                                                                                          |
|close()    | int fd                   | 关闭文件                                                                                                 |
|remove()   | const char *pathname     | 删除文件                                                                                                 |
|lseek()    | FILE *stream             | 设置读写偏移                                                                                             |
|           | long offset              |                                                                                                          |
|           | int whence               |                                                                                                          |
|read()     | int fd                   | 读取文件数据                                                                                             |
|           | void *buf                |                                                                                                          |
|           | size_t count             |                                                                                                          |
|write()    | int fd                   | 写文件数据                                                                                               |
|           | const void* buf          |                                                                                                          |
|           | size_t count             |                                                                                                          |
|stat()     | const char *pathname     | 获取文件的属性信息                                                                                       |
|           | struct stat *buf         |                                                                                                          |
|utime()    | const char * pathname    | 更改访问和修改时间                                                                                       |
|           | struct utimebuf * buf    |                                                                                                          |
|link()     | const char *oldpath      | 创建链接文件（硬链接）                                                                                   |
|           | const char *newpath      |                                                                                                          |
|unlink()   | const char * pathname    | 删除指定文件，如果该文件为最后的链接点，则文件会被删除。<br>如果为符号链接，则链接删除。                 |
|symlink()  | const char *oldpath      | 创建符号链接文件, oldpath 指定文件允许不存在。                                                            |
|           | const char *newpath      |                                                                                                          |
|truncate() | const char *pathname     | 截取文件内容，将 path 指定的文件大小改为参数 length 的大小，<br>如果原来文件比 length 大，则超过的部分会被删除。|
|           | off_t length             |                                                                                                          |
|mkdir()    | const char *pathname     | 创建目录文件                                                                                             |
|           | mode_t  mode             |                                                                                                          |
|rmdir()    | const char *pathname     | 删除目录文件                                                                                             |
|rename     | const char *pathname     | 更改文件名称                                                                                             |
|           | const char *newpathname  |                                                                                                          |
|chmod      | const char *pathname     | 更改文件权限                                                                                             |
|           | mode_t mode              |


API使用实例
====

下面实例演示了通过 API 在 guestdir 目录下创建了一个 testfile 文件并写入 testdata 内容。  

```lang-c++
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

static char testdata[] = "abcdefghijklmnopqrstuvwxyz";
static int testdatalen = sizeof(testdata) - 1;
#define testfile "/opt/sequoiadb/guestdir/testfile"

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



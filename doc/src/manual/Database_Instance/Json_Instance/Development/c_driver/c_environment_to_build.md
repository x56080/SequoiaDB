本文档主要介绍如何获取驱动开发包和配置开发环境。

##获取驱动开发包##

用户可以从 [SequoiaDB 巨杉数据库官网][download]下载对应操作系统版本的 SequoiaDB 驱动开发包。

##配置开发环境##

###Linux###

下述内容以在 SequoiaDB 的安装目录 `/opt/sequoiadb` 下配置 C 驱动为例，介绍具体操作步骤。

> **Note:**
>
> 需使用 root 用户权限配置开发环境。

1. 解压下载的驱动开发包，以 `C&CPP-3.4.2-linux_x86_64.tar.gz` 为例

    ```lang-bash
    # tar -zxvf C\&CPP-3.4.2-linux_x86_64.tar.gz
    ```

2. 将解压的目录拷贝至 SequoiaDB 的安装目录下

    ```lang-bash
    # cp -r C\&CPP-3.4.2-linux_x86_64 /opt/sequoaidb
    ```

3. 将目录 `C&CPP-3.4.2-linux_x86_64` 重命名为 `sdbdriver`

    ```lang-bash
    # mv /opt/sequoaidb/C\&CPP-3.4.2-linux_x86_64 /opt/sequoiadb/sdbdriver
    ```

4. 设置环境变量并使其生效

    ```lang-bash
    # echo "export LD_LIBRARY_PATH=/opt/sequoiadb/sdbdriver/lib" >> /etc/profile
    # source /etc/profile
    ```

###Windows###

暂未推出 Windows 驱动开发包



[^_^]:
    本文使用的所有引用和链接
[download]:http://download.sequoiadb.com/cn/driver

本文档主要介绍如何卸载 SequoiaFS 目录。  

##fsstop.sh卸载##

卸载脚本位于 SequoiaFS 安装目录的 `bin` 目录中。

* 指定挂载目录名称卸载

   ```lang-bash
   $ ./fsstop.sh -m /home/sdbadmin/guestdir
   ```

* 或使用别名卸载指定目录（--alias 参数指定挂载目录的别名）

   ```lang-bash
   $ ./fsstop.sh --alias guestdir
   ```
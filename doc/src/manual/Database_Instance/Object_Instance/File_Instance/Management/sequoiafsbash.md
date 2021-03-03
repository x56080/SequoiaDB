本文档主要介绍 fsstart.sh、fsstop.sh 和 fslist.sh 的使用方式。

> **Note:**
>
> fsstart.sh、fsstop.sh 和 fslist.sh 在 SequoiaFS 的 `bin` 目录下。

##fsstart.sh##

fsstart.sh 用于挂载目录，挂载目录前需保证配置文件路径下已经准备好配置文件，配置文件创建规则可参考[配置管理](manual/Database_Instance/Object_Instance/File_Instance/Management/sequoiafsconfig.md)。

###语法###

./fsstart.sh -c arg [options]

./fsstart.sh -m arg [options]

./fsstart.sh --alias arg [options]

./fsstart.sh -a [options]

###参数说明###

| 参数名 | 缩写 | 描述 |
| ----   | ---- | ---- |
| --help | -h   | 返回帮助信息 |
| --version | -v | 显示 SequoiaFS 的版本 |
| --confpath | -c | 指定配置文件所在路径，使用该路径中的配置文件及其它指定参数启动 SequoiaFS |
| --mountpoint | -m | 指定挂载目录启动，系统会根据挂载目录找到配置文件路径，使用该路径中的配置文件及其它指定参数启动 SequoiaFS |
| --alias |  | 指定挂载目录的别名，系统会根据别名找到配置文件路径，使用该路径中的配置文件及其它指定参数启动 SequoiaFS | 
| --all | -a | 启动所有挂载目录，启动脚本在默认配置路径下找到全部配置文件，分别启动 SequoiaFS；若启动时指定本参数，则 --confpath、--alias 和 --mountpoint 参数均无效 |

>**Note:**  
>
> SequoiaFS 启动时还可以增加其他参数，参数使用方法可参考[配置管理](manual/Database_Instance/Object_Instance/File_Instance/Management/sequoiafsconfig.md)。

###示例###

* 指定配置文件启动

   ```lang-bash
   $ ./fsstart.sh -c /opt/sequoiadb/tools/sequoiafs/conf/local/guestdir
   ```

* 指定别名启动

   ```lang-bash
   $ ./fsstart.sh --alias guestdir
   ```

* 指定挂载目录启动

   ```lang-bash
   $ ./fsstart.sh -m /home/sdbadmin/guestdir
   ```

* 启动所有挂载目录

   ```lang-bash
   $ ./fsstart.sh -a
   ```

##fsstop.sh##

fsstop.sh 用于卸载目录。

###语法###

./fsstop.sh -a

./fsstop.sh -m arg

./fsstop.sh --alias arg

###参数说明###

| 参数名 | 缩写 | 描述 |
| ----   | ---- | ---- |
| --help | -h | 返回帮助信息 |
| --version | -v | 显示 SequoiaFS 的版本 |
| --mountpoint | -m | 指定挂载目录，卸载指定挂载目录 |
| --alias | | 指定别名，卸载别名对应的挂载目录 |
| --all | -a | 卸载全部已挂载目录 |

###示例###

* 卸载全部挂载目录

   ```lang-bash
   $ ./fsstop.sh -a
   ```

* 指定别名卸载

   ```lang-bash
   $ ./fsstop.sh --alias guestdir
   ```

* 指定挂载目录卸载

   ```lang-bash
   $ ./fsstop.sh -m /home/sdbadmin/guestdir
   ```

##fslist.sh##

fslist.sh 用于查询挂载目录信息，默认显示挂载目录、别名和进程 PID。

###语法###

./fslist.sh

./fslist.sh [-l]

./fslist.sh [-l] [--detail]

./fslist.sh [-l] [--detail] [-m local]

./fslist.sh [-l] [--detail] [--alias arg]

###参数说明##

| 参数名 | 缩写 | 描述 |
| ----   | ---- | ---- |
| --help | -h | 返回帮助信息 |
| --version | -v | 显示 SequoiaFS 的版本 |
| --long | -l | 显示详细信息，包括 Collection 和 ConfPath |
| --mode | -m | 挂载目录信息显示模式，默认值为"run"，取值如下：<br>- "run"：只显示已挂载的挂载目录信息 <br> - "local"：只显示默认配置路径下已配置的挂载目录信息 |
| --alias | | 指定别名查询，只显示该别名相关信息 |
| --detail | | 显示挂载目录的配置信息 |

###示例###

* 查询全部已挂载的目录信息

   ```lang-bash
   $ ./fslist.sh -l
   ```

* 查询全部已挂载的目录及目录配置信息

   ```lang-bash
   $ ./fslist.sh -l --detail
   ```

* 指定别名查询已挂载的目录及目录配置信息

   ```lang-bash
   $ ./fslist.sh -l --detail --alias guestdir
   ```

* 查询配置路径中所有挂载目录信息，包括尚未启动的挂载目录

   ```lang-bash
   $ ./fslist.sh -l -m local
   ```
本章介绍 fsstart.sh，fsstop.sh，fslist.sh 几个命令的使用方式。

fsstart.sh, fsstop.sh, fslist.sh 在 SequoiaFS 的 bin 目录中。

##fsstart.sh##

挂载目录。

###用法###

./fsstart.sh [-c arg]  [options]

./fsstart.sh [-m arg]  [options]

./fsstart.sh [--alias arg]  [options]

./fsstart.sh [-a]  [options]

###参数说明###

**--help, -h**

返回帮助信息

**--confpath, -c**

指定配置文件所在路径，根据该路径中的配置文件及其它指定参数启动 SequoiaFS。

**--mountpoint, -m**

指定待挂载目录，并根据挂载目录找到配置文件路径，根据该路径中的配置文件及其它指定参数启动 SequoiaFS。

**--alias**

指定待挂载目录的别名，并根据别名找到配置文件路径，根据该路径中的配置文件及其它指定参数启动 SequoiaFS。

**--all, -a**

启动所有待挂载目录。启动脚本在默认配置路径下找到全部配置文件，分别启动 SequoiaFS。当启动时指定本参数，--confpath，--alias，--mountpoint，--logpath参数均无效。

##fsstop.sh##

卸载目录。

###用法###

./fsstop.sh [-a]

./fsstop.sh [-m arg]

./fsstop.sh [--alias arg]

###参数说明###

**--help, -h**

返回帮助信息

**--mountpoint, -m**

指定挂载目录，卸载指定挂载目录。

**--alias**

指定别名，卸载别名对应的挂载目录。

**--all, -a**

卸载全部已挂载目录。

##fslist.sh##

查询挂载目录信息，默认显示挂载目录，别名，和进程PID。

###用法###

./fslist.sh

./fslist.sh [-l]

./fslist.sh [-l] [--detail] 

./fslist.sh [-l] [--detail] [-m local] 

###参数说明###

**--help, -h**

返回帮助信息。

**--long, -l**

显示详细信息，包括Collection，ConfPath。

**--mode, -m**

参数范围：run，local。

- run: 只显示已挂载的挂载目录信息。   
- local：只显示默认配置路径下已配置的挂载目录信息。

默认值：run

**--detail**

显示挂载目录的配置信息。



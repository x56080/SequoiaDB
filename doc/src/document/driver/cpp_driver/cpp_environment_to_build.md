##获取驱动开发包##

从 [SequoiaDB](http://download.sequoiadb.com/cn/index-cat_id-2) 下载对应操作系统版本的 SequoiaDB 驱动开发包。

##配置开发环境##

* Linux

  （1）解压下来的驱动开发包；

  （2）将压缩包中的 driver 目录，拷贝到开发工程目录中（建议放在第三方库目录下），并命名为 sdbdriver。

  （3）将 sdbdriver/include 目录加入到编译头目录，并将 sdbdriver/lib 目录加入连接目录。

  **动态链接：**

  使用 lib 目录下的 libsdbcpp.so 动态库，g++ 编译参数形式如：

  ```lang-javascript
  $ g++ main.cpp -o test -I &lt;PATH&gt;/sdbdriver/include -L &lt;PATH&gt;/sdbdriver/lib -lsdbcpp
  ```

  其中：PATH 为 sdbdriver 放置路径；运行程序时，用户需要将 LD_LIBRARY_PATH 路径指定为包含 libsdbcpp.so 动态库的路径。

  ```lang-javascript
  $ export LD_LIBRARY_PATH=&lt;PATH&gt;/sdbdriver/lib
  ```

  >**Note:**
  >
  >如果运行程序时会出现错误提示：
  >
  >```
  >error while loading shared libraries: libsdbcpp.so: cannot open shared object file: No such file or directory
  >```
  >
  >表示没有正确设置 LD_LIBRARY_PATH，LD_LIBRARY_PATH 是环境变量，建议设置到 /etc/profile 或者应用程序的启动脚本中，避免每次新开终端都需要重新设置。

  **静态链接：**

  使用 lib 目录下的 libstaticsdbc.a 静态库，g++ 编译参数形式如：

  ```lang-javascript
  $ g++ main.c -o test -I &lt;path&gt;/sdbdriver/include –L &lt;path&gt;/sdbdriver/lib/ -  lstaticsdbcpp –lm -lpthread -ldl
  ```

* Windows

  暂未推出 Windows 驱动开发包。

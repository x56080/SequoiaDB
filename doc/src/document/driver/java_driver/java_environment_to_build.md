##获取驱动开发包##

从 [SequoiaDB](http://download.sequoiadb.com/cn/index-cat_id-2) 下载对应操作系统版本的 SequoiaDB 驱动开发包；解压驱动开发包，从 driver/java/ 目录中获取 sequoiadb.jar 文件。

##配置 Eclipse 开发环境##

（1）将 SequoiaDB 驱动开发包中的 sequoiadb.jar 文件拷贝到工程文件目录下（建议将其放置在其他所有依赖库目录，如 lib 目录）；

（2）在 Eclipse 界面中，创建/打开开发工程；

（3）在 Eclipse 主窗口左侧的“Package Explore”窗口中，选择开发工程，并点击鼠标右键；

（4）在菜单中选择“properties”菜单项；

（5）在弹出的“property for project …”窗口中，选择“Java Build Path”->“Libraries”，如下图所示：

![](driver/java_driver/eclipse.jpg)

（6） 点击“Add JARs..”按钮，选择添加 sequoiadb.jar 到工程中；

（7） 点击“OK”完成环境配置。

更多操作请参考 [Java 开发基础](driver/java_driver/java_development_foundation.md)

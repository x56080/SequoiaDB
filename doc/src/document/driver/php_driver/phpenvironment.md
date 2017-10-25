##获取驱动开发包##

从[SequoiaDB下载](http://download.sequoiadb.com/cn/index-cat_id-2)对应操作系统版本的SequoiaDB驱动开发包。

解压驱动开发包，选择libsdbphp-x.x.x.so（x.x.x 为版本号，请根据PHP版本选择，前两位需相同版本，第三位版本要小于或等于PHP的版本）。

##支持版本##

- **x86 Linux 和 Power Linux**

  | PHP版本 |
  | ------- |
  | 5.3.3、5.3.8、5.3.10、5.3.15 |
  | 5.4.6或更高 |
  | 5.5.x   |
  | 5.6.x   |
  | 7.0.x   |
  | 7.1.1   |

- **Windows**

  暂未提供Windows驱动开发包

##配置开发环境##

- **Linux**


 **准备工作：**安装Apache和PHP。

    
 **配置步骤：**


 **1.** 打开/etc/php5/apache2/php.ini文件；

      
 **2.** 在该文件的[PHP]配置段中新增如下行：


 ```
 extension=<PATH>/libsdbphp-x.x.x.so
 ```

 其中PATH为 libsdbphp-x.x.x.so 文件放置路径。

 **3.** 保存关闭文件；

 **4.** 重新启动apache2服务；

 ```
 $ service apache2 restart（SUSE/Redhat/Ubuntu） 或  service httpd restart（CentOS）
 ```

 **5.** 编写包含如下内容PHP测试脚本，保存为test.php文件，并放在apache的Web服务目录下；

 ```
 <?php phpinfo(); ?>
 ```

 **6.** 通过浏览器打开如下网址

 ```
 http://<IP>/test.php
 ```

 \<IP\>为apache所在的主机IP, 在打开的页面中查看是否包含SequoiaDB模块。

- **Windows**

 暂未提供Windows驱动开发包

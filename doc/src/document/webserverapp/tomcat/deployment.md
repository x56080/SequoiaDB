###环境准备###

安装pg，参考[pg部署](connector/postgresql/deployment.md)

###安装配置###

  
  1.下载tomcat安装包,可以去官网下载对应版本[http://tomcat.apache.org](http://tomcat.apache.org/download-70.cgi)

  2.使用root用户，将tomcat安装包放在/opt目录下，并解压安装包

```lang-bash
#tar -zxvf apache-tomcat-7.0.68.tar.gz
```
将解压后的文件拷贝到/usr/local下

```lang-bash
#cp -R /opt/apache-tomcat-7.0.68 /usr/local/
```

  3.打开/usr/local/apache-tomcat-7.0.68/bin/catalina.sh文件，增加如下配置项配置内存大小（根据项目实际需求进行修改）： 

```lang-bash
#vim /usr/local/apache-tomcat-7.0.68/bin/catalina.sh
```
```lang-ini
JAVA_OPTS="-server -Xms800m -Xmx800m -XX:PermSize=64M -XX:MaxNewSize=256m -XX:MaxPermSize=128m -Djava.awt.headless=true"
```

  4.查看tomcat端口是否被占用（默认是8080）

```lang-bash
#netstat -lnpt | grep 8080
```
如果端口被占用，可修改/usr/local/apache-tomcat-7.0.68/conf/server.xml文件的port参数的值，如下：

 ```lang-xml
<Connector port="8080" protocol="HTTP/1.1"
               connectionTimeout="20000"
               redirectPort="8443" />
```


  5.启动tomcat服务器

```lang-bash
#/usr/local/apache-tomcat-7.0.68/bin/startup.sh
```

  6.验证tmocat服务是否启动成功，访问http://ip:port,启动成功页面显示如下：


![tomcathome](webserverapp/tomcat/tomcathome.jpg)
1.将待发布的web应用war包放入tomcat服务器的/usr/local/apache-tomcat-7.0.68/webapps目录下，例如将实现连接pg获取pg版本的test应用打成test.war放入该目录下

2.重启tomcat使web应用加载成功

```lang-bash
#/usr/local/apache-tomcat-7.0.68/bin/shutdown.sh
#/usr/local/apache-tomcat-7.0.68/bin/startup.sh
```

3.在浏览器上输入http://ip:port/项目名，验证web应用是否发布成功，例如test应用发布成功时界面会显示pg数据库版本信息，如下图：

![](webserverapp/tomcat/webtest.jpg)


1. eclipse中创建Java web项目(如HelloEJB)，导出war包

   ![eclipse中创建Java web项目](webserverapp/jboss/web-0.png)


2. 进入 **Runtime** 的 **Deployments**界面中，点击 **Manage eployments**，点击 **Add Content**从本地选择HelloEJB.war包上传到jboss服务器

   ![上传war到jboss服务](webserverapp/jboss/web-2.png)


3. 项目上传到JBoss服务器后，选择HelloEJB点击 **Enable**启用

   ![启用war](webserverapp/jboss/web-3.png)


4. 在浏览器上输入http://ip:port/项目名，如http://192.168.31.8:8080/HelloEJB/，
网页上输出数据库的主版本号说明成功部署。

   ![查看运行效果](webserverapp/jboss/web-6.png)




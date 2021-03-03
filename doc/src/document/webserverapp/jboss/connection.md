##连接##

JBoss中数据连接配置其实就是配置数据源，在JBoss中配置数据源的方式有两种:根据创建模型来创建数据源、通过热部署来创建数据源(和部署web应用相似)，本次配置数据源用的方法就是热部署。因为此方法方便快捷，且比较灵活。

1. 复制数据库驱动

  将postgresql-42.0.0.jre7.jar复制到/opt/jboss/standalone/deployments/目录下

    ```lang-bash
    $ cp postgresql-42.0.0.jre7.jar /opt/jboss/standalone/deployments/
    $ ls /opt/jboss/standalone/deployments/
    ```
    ```
    postgresql-42.0.0.jre7.jar README.txt
    ```

2. 打开浏览器，地址为JBoss绑定的IP，访问端口默认9990，例如 http://192.168.31.8:9990
3. 输入用户名和密码，用户名是前面创建的JBoss后台用户
4. 点击 **登录** 按钮
5. 进入 **Runtime** 的 **Deployments**界面中，可以看到部署了但未启用的驱动，手动点击驱动后的 **Enable** 启用。

   ![登录后台管理界面](webserverapp/jboss/web-3.png)
 
6. 点击 **Add content**添加数据库 点击 **Enable**启用数据库驱动
   ![新增数驱动](webserverapp/jboss/web-3.png)

7. 创建JNDI，进入 **Profile**下的 **DataSources**点击 **Add** 创建JNDI

   ![创建JNDI](webserverapp/jboss/ds-2.png)
   ![创建JNDI](webserverapp/jboss/ds-3.png)

    >**Note:**
    >
    >在JBoss中JNDI的名字要以Java:jboss/ 开头。JNDI是一种由sun公司提供的Java命名系统接口，通过将名字与服务建立逻辑关联，从而通过不同的名字访问不同的服务。

8. 选择驱动，如果选择的驱动有多个，那根据自己的需求去选择,本次安装采用的是postgresql-42.0.0.jre7.jar

   ![选择驱动](webserverapp/jboss/ds-4.png)

9. 配置数据源连接

   ![创建数据源地址](webserverapp/jboss/ds-5.png)

   > **Note:**
   >
   >Connectiion url的格式为jdbc:postgresql://host:port/DBName，用户一定要严格按照此格式书写，此处用户名和密码必须在postgresql中存在。

10. 启用postDS数据源，点击 **Enable**启动数据源

   ![启用postDS数据源](webserverapp/jboss/ds-6.png)

11. 在 **Selection**下的  **Connection**中点击 **test connnection**,可以测试数据源是否成功配置。如下显示为配置成功。
   ![部署成功](webserverapp/jboss/ds-7.png)


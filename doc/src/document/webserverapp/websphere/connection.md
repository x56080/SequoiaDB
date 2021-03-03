##数据库连接配置##
服务启动成功后，通过浏览器登录控制台，输入 **用户标识** 和 **密码**，点击 **登录**

   > **Note:**  
   > 1）URL中的IP必须为 websphere 服务器 IP；  
   > 2）用户标识和密码为安装时设置的用户名和密码；   
   > 3）主机名以 suse113-3Node01 为例；

   ![](webserverapp/websphere/ds_1.jpg)  

##创建JDBC提供程序##

1. 选择 **资源**，选择 **JDBC**，选择 **JDBC提供程序**， **作用域** 下拉列表选择“节点=suse113-2Node01”

   ![](webserverapp/websphere/ds_2.jpg)

2. 点击 **新建**

   ![](webserverapp/websphere/ds_3.jpg)

3.  **数据库类型**选择“用户定义的”， **实现类名**输入“org.postgresql.jdbc2.optional.ConnectionPool”， **名称**输入“sdb JDBC Provider”，点击 **下一步**

   ![](webserverapp/websphere/ds_4.jpg)

4. 输入 “/opt/postgresql-9.3-1102.jdbc41.jar”（文件在服务器 /opt 目录下必须存在），点击 **下一步**

   ![](webserverapp/websphere/ds_5.jpg)

5. 点击 **完成**

   ![](webserverapp/websphere/ds_6.jpg)

##创建数据源##

1. 点击 **sdb JDBC Provider**

   ![](webserverapp/websphere/ds_7.jpg)

2. 进入 sdb JDBC Provider 页面，点击 **数据源**

   ![](webserverapp/websphere/ds_8.jpg)

3. 进入数据源页面，点击 **新建**

   ![](webserverapp/websphere/ds_9.jpg)

4. 数据源名输入“sdb DataSource”，JNDI名称中输入“jdbc/sdb DataSource”，点击 **下一步**

   ![](webserverapp/websphere/ds_10.jpg)

5. 点击 **下一步**

   ![](webserverapp/websphere/ds_11.jpg)

6. 点击 **下一步**

   ![](webserverapp/websphere/ds_12.jpg)

7. 点击 **完成**

   ![](webserverapp/websphere/ds_13.jpg)

8. 点击 **sdb DataSource**

   ![](webserverapp/websphere/ds_28.jpg)

9. 点击 **JAAS － J2C 认证数据**

   ![](webserverapp/websphere/ds_14.jpg)

10. 点击 **新建**

   ![](webserverapp/websphere/ds_15.jpg)

11. 输入别名、用户标识、密码， 点击 **确定**  

   > **Note:**  
   > 1）这里的用户标识和密码是安装 SequoiaSQL 时的用户名和密码；   

   ![](webserverapp/websphere/ds_16.jpg) 

12. 点击 **sdb DataSource**，返回上一级页面

   ![](webserverapp/websphere/ds_17.jpg)

13. 点击 **定制属性**，进入定制属性页面

   ![](webserverapp/websphere/ds_19.jpg)

14. 选择配置“databaseName”、“password”、“portNumber”

   ![](webserverapp/websphere/ds_20.jpg)

15. 翻页，选择配置“serverName”、“user”

   ![](webserverapp/websphere/ds_21.jpg)

16. 点击属性名（如“databaseName”）进入配置页面配置对应属性值，修改后显示如下：

   ![](webserverapp/websphere/ds_22.jpg)

17. 点击 **保存** 到主配置，保存配置

   ![](webserverapp/websphere/ds_23.jpg)

18. 点击 **数据源** ，勾选新建的数据源，点击 **测试连接**

   ![](webserverapp/websphere/ds_24.jpg)

19. 在页面顶部显示测试连接成功，数据源配置成功。

   ![](webserverapp/websphere/ds_25.jpg)



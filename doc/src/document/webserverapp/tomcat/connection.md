1.部署web应用使用的JNDI数据源，将pg对应的驱动jar包放到tomcat服务器的/usr/local/apache-tomcat-7.0.68/lib目录下，可以去官网下载对应版本 [http://jdbc.postgresql.org/download.html](http://jdbc.postgresql.org/download.html)

  2.配置JNDI,在/usr/local/apache-tomcat-7.0.68/conf/context.xml文件中新增内容如下：

```lang-xml
<Resource 
         name="jdbc/pg"
         auth="Container"
         type="javax.sql.DataSource"
         maxActive="100"
         maxIdle="30"
         maxWait="10000"
         username="sdbadmin"
         password="sdbadmin"
         driverClassName="org.postgresql.Driver"
         url="jdbc:postgresql://localhost:5432/foo"/>
```

>**Note:** 
>
> name：表示以后要查找的名称。通过此名称可以找到DataSource，此名称任意更换，但是程序中最终要查找的就是此名称，为了不与其他的名称混淆，所以使用jdbc/pg，现在配置的是一个jdbc的关于pg的命名服务。<br/>
  auth：由容器进行授权及管理，指的用户名和密码是否可以在容器上生效。<br/>
  type：此名称所代表的类型，现在为javax.sql.DataSource。<br/>
  maxActive：表示一个数据库在此服务器上所能打开的最大连接数。<br/>
  maxIdle：表示一个数据库在此服务器上维持的最小连接数。<br/>
  maxWait：最大等待时间。10000毫秒。<br/>
  username：数据库连接的用户名。<br/>
  password：数据库连接的密码。<br/>
  driverClassName：数据库连接的驱动程序。<br/>
  url：数据库连接的地址。<br/>

 3.重启tomcat使配置参数生效

```lang-bash
#/usr/local/apache-tomcat-7.0.68/bin/shutdown.sh
#/usr/local/apache-tomcat-7.0.68/bin/startup.sh
```
##环境准备##

首先，下载需要的安装包，对应版本参考如下：

软件| 版本
---|---
[postgresql]( https://www.postgresql.org/download/) | Postgresql9.3.4.tar.gz
[postgresql-JDBC]( https://jdbc.postgresql.org/) | postgresql-42.0.0.jre7.jar
[jboss](http://jbossas.jboss.org/downloads/) | jboss-as-7.1.1.Final.zip
[JDK](https://www.oracle.com/index.html/) | JDK1.7

	
 >**Note:**
 >
 >因为JBoss7与jdk1.8不兼容，建议使用jdk-1.7.x的版本，postgresql-JDBC对JDK的驱动依赖很强，不同的JDK版本对应的postgresql-JDBC的版本不一样，请参看下载官网上说明。


##安装配置##

1. postgresql的安装部署

 参照[Postgresql部署](connector/postgresql/deployment.md)

2. 配置JDK环境变量


 ```lang-bash
 $ vim /etc/profile
 ```
 增加如下配置项

 ```lang-ini
   export JAVA_HOME=/usr/java/jdk1.7.0_67
   export CLASSPATH=${JAVA_HOME}/lib:${JAVA_HOME}/jre/lib
   export PATH=${JAVA_HOME}/bin:${PATH}
 ```
 使环境变量生效

 ```lang-bash
 $ source /etc/profile
 ```

3. 安装JBoss

 1. 将安装包解压到/opt目录下，并修改名字为jboss

      ```lang-bash 
      $ unzip jboss-as-7.1.1.Final.zip -d /opt
      $ mv JBoss-7.1.1.Final jboss
      ```

     > **Note:**
     >
     >JBoss的运行模式有两种，domain和standalone，两种模式的安装和配置是相同的，因为standlone提供的功能相对比较多，比如应用热部署，丰富的web操作界面等，为了降低安装的复杂度，在本次安装中采用的运行模式是standalone。

   2. 修改standlone.conf配置文件

      ```lang-bash
      $ cd /opt/jboss/bin
      $ vim standalone.conf
      ```
     设置JAVA_HOME的值（根据实际配置）

     ```lang-ini
     JAVA_HOME=/usr/java/jdk1.7.0_67
     ```

   3. 修改standlone.xml配置文件

     ```lang-bash
     $ cd /opt/jboss/standalone/configuration/
     $ vim standalone.xml
     ```
     将所有的127.0.0.1修改成本机IP，该实例的IP是192.168.31.8，在vim的命令行模式下输入

     ```lang-bash
     :%s/127.0.0.1/192.168.31.8/g		
     ```
    > **Note:**
    >
    >JBoss作为一个Web容器，不仅自身实现一些Java EE的规范，比如JMX，JMS，JTA，EJB等。它的主要作用是对外提供服务，比如OSGI功能等。所以建议将所有的地址配置在非127的网段里。

    4. 启动数据库

        进入bin目录执行standalone\.sh

       ```lang-bash
       $ cd /opt/jboss/bin
       $ ./standalone.sh
       ```

     >**Note:**
     >
     >执行standalone.sh脚本启动JBoss，所有的日志信息会输出到屏幕上；建议配置日志文件，根据日志的级别来选择日志是持久化还是重定向到/dev/null中。本次安装选择直接启动standalone .sh,目的是安装过程中能够及时查看信息，环境安装成功后可以根据自己需求来处理日志。

4. 添加JBoss后台访问用户

    进入bin目录执行add-user.sh

    ```lang-bash
    $ cd /opt/jboss/bin
    ./add-user.sh
    ```
    程序提示选择要创建的用户类型，如输入a

    ```
    What type of user do you wish to add? 
 a) Management User (mgmt-users.properties) 
 b) Application User (application-users.properties)
(a): a    							
    ```
    提示输入创建用户的角色、用户名和密码（默认创建ManagementRealm角色）

    ```
    Enter the details of the new user to add.
Realm (ManagementRealm) : 
Username (jboss) : jboss
Password : 
Re-enter Password : 
About to add user 'jboss' for realm 'ManagementRealm'
Is this correct yes/no? yes
    ```
    提示创建成功

    ```
   Added user 'jboss' to file    '/opt/jboss/standalone/configuration/mgmt-users.properties'
   Added user 'jboss' to file '/opt/jboss/domain/configuration/mgmt-users.properties'
 
   ```

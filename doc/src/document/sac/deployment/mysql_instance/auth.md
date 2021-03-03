
1. 为 MySQL 数据库实例设置鉴权。

   > **Note：**  
   > 为了保证安全，生产环境请不要使用简单密码。

   ```lang-bash
   $ /opt/sequoiasql/mysql/bin/mysql -h 127.0.0.1 -P 3306 -u root
   ```

   * 创建新用户，演示的用户名 sac，密码 123

      ```lang-sql
      mysql> grant all privileges on *.* to "sac"@"%" identified by "123";
      mysql> flush privileges;
      ```

   * 修改 root 的密码，演示修改密码为 123

     ```lang-sql
     mysql> update mysql.user set authentication_string = password("123") where user = "root";
     mysql> flush privileges;
     ```


2. 在 SAC 设置 MySQL 数据库实例的鉴权，进入 **部署 - 数据库实例** 页面。

   ![设置鉴权](sac/deployment/mysql_instance/auth_1.png)

3. 点击存储集群的 **鉴权**，输入 **用户名** 和 **密码**，点击 **确定** 按钮。

   ![设置鉴权](sac/deployment/mysql_instance/auth_2.png)

   ![设置鉴权](sac/deployment/mysql_instance/auth_3.png)

4. 进入 **数据 - 数据库实例** 页面，访问 MySQL 数据库实例的信息。

   ![设置鉴权](sac/deployment/mysql_instance/auth_4.png)
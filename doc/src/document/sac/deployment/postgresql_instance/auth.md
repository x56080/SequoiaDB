
1. 为 PostgreSQL 数据库实例设置鉴权。

   > **Note：**  
   > 为了保证安全，生产环境请不要使用简单密码。  
   > PostgreSQL 默认用 sdbadmin 账号访问，这跟 SAC 创建集群的用户名相关

   * 创建新用户，演示的用户名 sac，密码 123

     ```lang-bash
     $ su sdbadmin
     $ /opt/sequoiasql/postgresql/bin/psql -p 5432 postgres
     ```

     ```lang-sql
     postgres=# create user sac with password '123';
     ```

   * 修改 PostgreSQL 数据库实例的客户端认证配置文件，配置文件在数据路径下，演示的路径是 ```/opt/sequoiasql/postgresql/database/5432/pg_hba.conf```

     ```lang-bash
     $ vi /opt/sequoiasql/postgresql/database/5432/pg_hba.conf
     ```

     配置内容修改前：

     ```
     ...
     # TYPE  DATABASE        USER            ADDRESS                 METHOD

     # "local" is for Unix domain socket connections only
     local   all             all                                     trust
     # IPv4 local connections:
     host    all             all             127.0.0.1/32            trust
     host    all             all             0.0.0.0/0               trust
     # IPv6 local connections:
     host    all             all             ::1/128                 trust
     ...
     ```

     配置内容修改后，表示使用 TCP 连接的需要携带 md5 算法加密的密码认证：

     ```
     ...
     # TYPE  DATABASE        USER            ADDRESS                 METHOD

     # "local" is for Unix domain socket connections only
     local   all             all                                     trust
     # IPv4 local connections:
     host    all             all             127.0.0.1/32            trust
     host    all             all             0.0.0.0/0               md5
     # IPv6 local connections:
     host    all             all             ::1/128                 trust
     ...
     ```

   * 让 PostgreSQL 数据库实例重新加载配置，演示的实例名是 PostgreSQLInstance1。

     ```lang-bash
     /opt/sequoiasql/postgresql/bin/sdb_pg_ctl reload PostgreSQLInstance1
     ```

2. 在 SAC 设置 PostgreSQL 数据库实例的鉴权，进入 **部署 - 数据库实例** 页面。

   ![设置鉴权](sac/deployment/postgresql_instance/auth_1.png)

3. 点击 PostgreSQL 数据库实例的 **鉴权**，输入 **用户名** 和 **密码**，点击 **确定** 按钮。

   ![设置鉴权](sac/deployment/postgresql_instance/auth_2.png)

4. 进入 **数据 - 数据库实例** 页面，访问 PostgreSQL 数据库实例的信息。

   ![设置鉴权](sac/deployment/postgresql_instance/auth_3.png)
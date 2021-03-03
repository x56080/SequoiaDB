
1. 为存储集群创建鉴权，演示的鉴权用户名 root，密码 123。

   > **Note：**  
   > 为了保证安全，生产环境请不要使用简单密码。

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   ```

   ```lang-javascript
   > var db = new Sdb( "localhost", 11810 )
   > db.createUsr( "root", "123" )
   ```

2. 在 SAC 设置存储集群的鉴权，进入 **部署 - 分布式存储** 页面，存储集群报错：```Authority is forbidden，错误码 -179```。

   ![设置鉴权](sac/deployment/distributed_storage/sdb_auth_1.png)

3. 点击存储集群的 **鉴权**，输入 **用户名** 和 **密码**。

   ![设置鉴权](sac/deployment/distributed_storage/sdb_auth_2.png)

4. SAC 显示恢复正常。

   ![设置鉴权](sac/deployment/distributed_storage/sdb_auth_3.png)

- 为集群创建鉴权。
   1. 执行 sdb shell，sdb shell 的路径在 SequoiaDB 安装的bin目录中，演示是默认路径。

      ```lang-javascript
      $ cd /opt/sequoiadb/bin
      $ ./sdb
      Welcome to SequoiaDB shell!
      help() for help, Ctrl+c or quit to exit
      ```
   2. 连接到集群，创建用户名和密码。演示是在 coord 节点的主机上，创建用户名 admin，密码 123。

      ```lang-javascript
      > db = new Sdb( "localhost", 11810 )
      localhost:11810
      Takes 0.101142s.
      > db.createUsr( "admin", "123" )
      Takes 0.394340s.
      ```

- 在 SAC 设置业务鉴权
   1. 在部署首页，创建鉴权前。

      ![设置鉴权](sac/deployment/sdb_auth_1.jpg)
   2. 创建鉴权后，业务报错：Authority is forbidden，错误码-179。

      ![设置鉴权](sac/deployment/sdb_auth_2.jpg)
   3. 点击业务的 **鉴权**，在 **设置鉴权** 窗口输入在 sdb shell 创建的用户名和密码，点击 **确定**。

      ![设置鉴权](sac/deployment/sdb_auth_3.jpg)
   4. 业务恢复正常。

      ![设置鉴权](sac/deployment/sdb_auth_4.jpg)
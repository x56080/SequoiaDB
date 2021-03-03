
1. 演示已经创建 foo.bar 的集合，
   集合创建可以参考[创建集合文档](sac/data_operation/collection/create_cl.md)。

   ![Lob](sac/data_operation/lob_1.jpg)

2. 使用 coord 节点的主机，运行 sdb shell 导入Lob：

   ```lang-javascript
   $ /opt/sequoiadb/bin/sdb
   Welcome to SequoiaDB shell!
   help() for help, Ctrl+c or quit to exit
   > db = new Sdb( "localhost", 11810 )
   localhost:11810
   Takes 0.1512s.
   > db.foo.bar.putLob( '/opt/pic.jpg' )
   5878b0add9d765d278000000
   Takes 2.2545s.
   ```

3. 导入完成。

   ![Lob](sac/data_operation/lob_2.jpg)

1. 创建 **foo.bar** 集合，创建集合可以参考 [创建集合文档](sac/sequoiadb_data/collection/create_cl.md)。

   ![Lob](sac/sequoiadb_data/lob_1.png)

2. 运行 sdb shell 导入 Lob：

   ```lang-bash
   $ /opt/sequoiadb/bin/sdb
   ```

   ```lang-javascript
   > db = new Sdb( "localhost", 11810 )
   > db.foo.bar.putLob( '/opt/pic.jpg' )
   ```

3. 导入完成，在 SAC 查看。

   ![Lob](sac/sequoiadb_data/lob_2.png)
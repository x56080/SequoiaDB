
1. 点击导航 **数据 - 分布式存储** 的名字，进入 **集合** 分页。演示已经创建 **foo.bar** 集合。

   集合创建可以参考[创建集合文档](sac/sequoiadb_data/collection/create_cl.md)。

   ![插入](sac/sequoiadb_data/record/insert_1.png)

2. 选择 **foo.bar** 集合，点击 **浏览数据**，进入数据操作页面。

   ![插入](sac/sequoiadb_data/record/insert_2.png)

3. 点击 **插入** 按钮，通过 JSON 图形化界面构造一条记录，点击 **确定** 按钮。

   ```lang-json
   {
      "name": "Jack",
      "phone": "136123456",
      "address": "Guangzhou City, Guangdong Province"
   }
   ```

   ![插入](sac/sequoiadb_data/record/insert_3.png)

4. 记录插入成功。

   ![插入](sac/sequoiadb_data/record/insert_4.png)

5. 点击 **插入** 按钮，打开插入记录的窗口，点击 **A** 图标，切换到字符串模式，
   输入一个标准 JSON 的字符串，点击 **确定** 按钮。

   ```lang-json
   {
      "name": "Mike",
      "phone": "136456789",
      "address": "Guangzhou City, Guangdong Province"
   }
   ```

   ![插入](sac/sequoiadb_data/record/insert_5.png)

7. 记录插入成功。

   ![插入](sac/sequoiadb_data/record/insert_6.png)
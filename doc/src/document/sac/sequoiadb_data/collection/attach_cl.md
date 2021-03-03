
演示模拟创建一个保存3年时间的交易流水表。

1. 创建 ```statement``` 集合空间，集合空间创建可以参考 [创建集合空间文档](sac/sequoiadb_data/collection_space/create_cs.md)。

   ![挂载集合](sac/sequoiadb_data/collection/attach_cl_1.png)

2. 创建一个垂直分区的集合 ```history```，分区键取决于实际业务场景，演示取 ```time``` 字段。

   ![挂载集合](sac/sequoiadb_data/collection/attach_cl_2.png)

3. 创建3个普通集合：```2017```、```2018```和```2019```，分别用于存储3年的交易流水账单。

   ![挂载集合](sac/sequoiadb_data/collection/attach_cl_3.png)

4. 创建完成。

   ![挂载集合](sac/sequoiadb_data/collection/attach_cl_4.png)

5. 点击 **挂载集合**，集合选择 ```statement.history```，分区选 ```statement.2017```。分区范围的字段名 ```time```，类型选 ```Date```，范围按照时间划分，点击 **确定**。

   ![挂载集合](sac/sequoiadb_data/collection/attach_cl_5.png)

6. 集合 ```2018``` 和 ```2019``` 也挂载上去。

   ![挂载集合](sac/sequoiadb_data/collection/attach_cl_6.png)

7. 点击表格的 **statement.history**，在集合属性 Partitions 点击 **显示** 按钮。

   ![挂载集合](sac/sequoiadb_data/collection/attach_cl_7.png)

在集合 **statement.history** 中插入两条记录测试，两条记录分别分布在 **statement.2017** 和 **statement.2019** 中

![挂载集合](sac/sequoiadb_data/collection/attach_cl_8.png)

![挂载集合](sac/sequoiadb_data/collection/attach_cl_9.png)

> **数据切分相关文档：**  
> 1. 分区类型 [点击查看](basic_operation/sharding/overview.md)  
> 2. 分区键   [点击查看](basic_operation/sharding/shardingkey.md)  
> 3. 分区集合 [点击查看](basic_operation/sharding/sharding_collection.md)  
> 4. 分区索引 [点击查看](basic_operation/sharding/sharding_index.md)   
> 5. 数据切分 [点击查看](basic_operation/sharding/data_split.md)


1. 点击导航 **数据 - 分布式存储** 的名字，进入 **集合空间** 分页，创建 **statement** 的集合空间，集合空间创建可以参考 [创建集合空间文档](sac/sequoiadb_data/collection_space/create_cs.md)。

   ![切分数据](sac/sequoiadb_data/collection/split_1.png)

2. 点击 **创建集合**，集合类型选择 **水平范围分区** 或 **水平散列分区**，演示选择 **水平范围分区**，填好参数，点击 **确定** 按钮。

   ![切分数据](sac/sequoiadb_data/collection/split_2.png)

3. 创建集合完成。

   ![切分数据](sac/sequoiadb_data/collection/split_3.png)

4. 点击 **切分数据**，选择切分的集合，填好参数，点击 **确定** 按钮。

   ![切分数据](sac/sequoiadb_data/collection/split_4.png)

6. 完成后，在点击集合属性 Partitions 的 **显示** 按钮，现在 **transfer 集合** 分布在 group1 和 group2 分区组上。

   ![切分数据](sac/sequoiadb_data/collection/split_5.png)

> **Note:**  
> 文档仅仅演示切分数据的步骤。  
> 数据应该根据实际场景来做切分。
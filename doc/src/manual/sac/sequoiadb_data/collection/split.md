
> **数据切分相关文档：**  
>- [分区类型](manual/Distributed_Engine/Architecture/Sharding/architecture.md)  
> - [分区键选择](manual/Distributed_Engine/Architecture/Sharding/sharding_keys.md)   
> - [分区配置](manual/Distributed_Engine/Architecture/Sharding/config.md)   
> - [多维分区](manual/Distributed_Engine/Architecture/Sharding/multi_dimension.md)


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

| 相关文档 |
| -------- |
| [分区类型](basic_operation/sharding/overview.md)            |
| [分区键](basic_operation/sharding/shardingkey.md)           |
| [分区集合](basic_operation/sharding/sharding_collection.md) |
| [分区索引](basic_operation/sharding/sharding_index.md)      |
| [数据切分](basic_operation/sharding/data_split.md)          |

1. 演示已经创建 foo 的集合空间，集合空间创建可以参考[创建集合空间文档](sac/data_operation/collection_space/create_cs.md)。

   ![切分数据](sac/data_operation/collection/split_1.jpg)

2. 点击 **创建集合**，集合类型选择 **水平范围分区** 或 **水平散列分区**，演示选择 **水平散列分区**，输入集合名，输入分区键，点击 **确定**。

   ![切分数据](sac/data_operation/collection/split_2.jpg)

3. 创建集合完成。

   ![切分数据](sac/data_operation/collection/split_3.jpg)

4. 点击集合属性 Partitions 的 **显示** 按钮，演示的 hash_cl 集合在 group1 分区组中。

   ![切分数据](sac/data_operation/collection/split_4.jpg)

5. 点击 **切分数据**，选择切分的集合，源分区组选第3步的 group1 分区组，目标分区组演示中选择 group2，点击 **确定**。

   ![切分数据](sac/data_operation/collection/split_5.jpg)

6. 完成后，在点击集合属性 Partitions 的 **显示** 按钮，现在 group1 和 group2 各一半。

   ![切分数据](sac/data_operation/collection/split_6.jpg)

> **Note:**  
> 文档仅仅演示切分数据的步骤。  
> 切分数据需要根据实际业务来调整参数。
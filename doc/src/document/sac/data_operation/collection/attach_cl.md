
1. 演示已经创建 foo 的集合空间，集合空间创建可以参考[创建集合空间文档](sac/data_operation/collection_space/create_cs.md)。

   ![挂载集合](sac/data_operation/collection/attach_cl_1.jpg)

2. 创建一个垂直分区的集合 main，分区键输入 name，分区键需要根据实际业务，演示取 name 字段。

   ![挂载集合](sac/data_operation/collection/attach_cl_2.jpg)

3. 创建一个普通集合 bar，也可以是水平范围分区或者水平散列分区的集合。

   ![挂载集合](sac/data_operation/collection/attach_cl_3.jpg)

4. 创建完成。

   ![挂载集合](sac/data_operation/collection/attach_cl_4.jpg)

5. 点击 **挂载集合**，集合选垂直分区 foo.main，分区选 foo.bar。分区范围 字段名输入 main的分区键，范围根据实际业务，演示取 1 - 100，点击 **确定**。

   ![挂载集合](sac/data_operation/collection/attach_cl_5.jpg)

6. 挂载完成。

   ![挂载集合](sac/data_operation/collection/attach_cl_6.jpg)

7. 点击表格的 **foo.main**，在集合属性Partitions点击 **显示** 按钮。

   ![挂载集合](sac/data_operation/collection/attach_cl_7.jpg)
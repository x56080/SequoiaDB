SequoiaDB 与 Hadoop 在物理上部署方案建议如下图所示：

 * SequoiaDB 与 Hadoop 部署在相同的物理设备上，以减少 Hadoop 与 SequoiaDB 之间的网络数据传输。

 * 每个物理设备上各部署一个协调节点和多个数据节点，编目节点可选在任意三台物理设备各部署一个编目节点。

![hadoop](connector/hadoop/hadoop.jpg)

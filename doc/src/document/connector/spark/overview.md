Apache的Spark是一个高速的通用集群式计算系统。Spark是一个可扩展的数据分析平台，该平台集成了原生的内存计算，因此它在使用中相比Hadoop 的集群存储来说，会有不少的性能优势。

Apache Spark提供了高级的Java、Scala和Python APIs，同时还拥有优化的引擎来支持常用的执行图。Spark
还支持多样化的高级工具，其中包括了处理结构化数据和SQL的SparkSQL，处理机器学习的MLlib，图形处理的 GraphX，还有SparkStreaming。

##Spark组成##

在集群中，Spark应用以独立的进程集合的方式运行，并由主程序（driver program）中的SparkContext
对象进行统一的调度。当需要在集群上运行时，SparkContext会连接到几个不同类的ClusterManager（集群管理器）上（Spark 自己的Standalone/Mesos/YARN）, 集群管理器将给各个应用分配资源。连接成功后，Spark
会请求集群各个节点的Executor（执行器），它是为应用执行计算和存储数据的进程的总称。之后，Spark会将应用提供的代码（应用已经提交给 SparkContext 的JAR或Python文件）交给executor。最后，由SparkContext 发送tasks提供给其执行。

![](connector/spark/spark_components_en.jpg)

关于这个架构的几点介绍：

1.  每一个应用有其独立的Executor进程，这些进程将会在应用整个生命周期内为应用服务，并且会在多个线程中执行任务tasks。这种做法能有效的隔离不同的应用，在调度和执行端都能很好的隔离（每个驱动调度自己的任务，不同的任务在不同的JVM中执行）。但是，这也意味着，如果不写入外部的存储设备，那数据就不能在不同的Spark 应用（SparkContext 实例）之中共享。
2.  Spark 对于下列的集群管理者是不可知的：只要Spark 能请求executor进程，且这些进程之间能互相通信，那么他就相对容易的去运行支持其他应用的集群管理器（如Mesos/YARN）。
3.  因为驱动在集群中调度任务，它将在worker nodes（工作节点）附近运行，最好是在相同的局域网当中。如果你不喜欢远程向集群发送请求，那么最好为驱动打开一个RPC然后让其在附近提交操作而不是在远离worker nodes 处运行驱动。

##Spark-SequoiaDB连接组件##

通过使用Spark-SequoiaDB连接组件，SequoiaDB可以作为Spark的数据源，从而可以通过SparkSQL实例对SequoiaDB数据存储引擎的数据进行查询、统计操作。




前提条件：
   1、整个集群必须分布在2台及以上的机器上，并且能够将这些机器划分成2个分组（或子网）SUB1和SUB2。
   2、编目分区组的节点必须在SUB1和SUB2的机器上都进行部署；
   3、协调节点必须在SUB1和SUB2的机器上都进行部署；
   4、同一个数据分区组中的节点必须在SUB1和SUB2的机器上都进行部署；

split和merge操作步骤和场景：
   1、在SUB1和SUB2上各选一台机器SUB1-NodeA和SUB2-NodeA(SUB1和SUB2内都有编目节点)；

   2、根据实际SequoiaDB的安装路径修改上述SUB1-NodeA和SUB2-NodeA中 init.sh、split.sh、merge.sh 中 SEQPATH 变量；

   3、请根据实际配置修改上述SUB1-NodeA和SUB2-NodeA中 cluster_opr.js 的参数定义部分，主要参数说明如下（格式在 cluster_opr.js 中有相应定义）：
      USERNAME:  登入所有机器的用户名（所有机器用户名密码需要统一）
      PASSWD:    登入所有机器用户名对应的密码
      SDBUSERNAME:登入数据库的用户名（如果数据库没有开启用户鉴权，则可以不填）
      SDBPASSWD：登入数据库的用户名对应的密码（如果数据库没有开启用户鉴权，则可以不填）
      SUB1HOSTS: SUB1的机器列表
      SUB2HOSTS: SUB2的机器列表
      COORDADDR: 协调节点定义，如果协调节点已经在协调节点组信息中，则此处填写一个可用地址即可
      CURSUB :   当前脚本所处的是在SUB1还是SUB2（注意，该参数非常重要）
      ACTIVE :   当前子网是否为激活状态。如果取false，则在split后，当前子网的集群为只读状态。
      NEEDREELECT：执行init动作时是否重新选主，在split和merge场景中init时可能需要让主节点在主数据中心的几个主机上，所以需要设置为true。
      NEEDBROADCASTINITINFO: 是否将init文件分发到集群的所有主机上。在split和merge场景中，需要分别去主备数据节点做init操作（除了保存集群信息，还有设置节点权值重新选举等动作），所以一般设置为false即可。
   4、分别在上述SUB1-NodeA和SUB2-NodeA的机器上的shell下执行 ' sh init.sh '，进行初始化（该初始化主要是保存当前集群所有的组信息，用于merge时恢复集群）

   5、当SUB1和SUB2出现了网络分离，相互无法访问时，此时可以分别在上述SUB1-NodeA和SUB2-NodeA的机器上的shell下执行 ' sh split.sh ' 进行集群分离， 让SUB1和SUB2分离成独立集群，此时 ACTIVE配置为true的子网可以对外提供读写操作，ACTIVE配置为false的子网只提供读操作；

   6、当子网1和子网2网络连通时，可以分别在上述SUB1-NodeA和SUB2-NodeA的机器上的shell下执行 ' sh merge.sh ' 进行集群合并， 把SUB1和SUB2独立的集群重新合成一个大群集。


detachGroupNode和attachGroupNode操作步骤和场景：
   1、根据实际SequoiaDB的安装路径修改 init.sh、detachGroupNode.sh、attachGroupNode.sh 中 SEQPATH 变量；
   
   2、请根据实际配置修改 cluster_opr.js 的参数定义部分，主要参数说明如下（格式在 cluster_opr.js 中有相应定义）：
      USERNAME:  登入所有机器的用户名（所有机器用户名密码需要统一）
      PASSWD:    登入所有机器用户名对应的密码
      SDBUSERNAME:登入数据库的用户名（如果数据库没有开启用户鉴权，则可以不填）
      SDBPASSWD：登入数据库的用户名对应的密码（如果数据库没有开启用户鉴权，则可以不填）
      SUB1HOSTS: 填本机主机名即可
      COORDADDR: 协调节点定义，如果协调节点已经在协调节点组信息中，则此处填写一个可用地址即可
      MINREPLICANUM: 剔除故障组节点后剩余的最小副本数, 若剔除后剩余副本数小于最小副本数，将不会执行剔除操作。
      NEEDREELECT：执行init动作时是否重新选主，在detachGroupNode和attachGroupNode的场景中，在初始化中一般不需要重新选主，设置为false即可。
      NEEDBROADCASTINITINFO: 是否将init文件分发到集群的所有主机上，在detachGroupNode和attachGroupNode的场景中，一般设置为true,这样无需到每台机器上重复做init操作。
   3、在准备做detachGroupNode和attachGroupNode的机器上的shell下执行 ' sh init.sh '，进行初始化(通过设置NEEDREELECT为false,可以让初始化时不重新选主)
   
   4、当集群中的部分节点发生故障导致复制组不可用时，选一台执行过sh init.sh 的机器，执行 ' sh detachGroupNode '剔除不可用节点。
   
   5、当故障节点恢复后，选一台执行过sh init.sh 的机器，执行 ' sh attachGroupNode '将节点重新加入对应复制组中。
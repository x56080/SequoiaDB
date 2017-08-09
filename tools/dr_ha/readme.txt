1、请根据实际配置修改 cluster_opr.js 的参数定义部分，主要参数说明如下：
   USERNAME: 登入所有机器的用户名（所有机器用户名需要统一）
   PASSWD: 登入所有机器用户名对应的密码
   SUB1HOSTS: 子网1的机器列表
   SUB2HOSTS: 子网2的机器列表
   SUB1CATAADDR: 子网1的编目列表（注意，这里的 SvcName 为本地平面的SvcName, 即 sdblist 显示的端口号）
   SUB2CATAADDR: 子网2的编目列表
   CURSUB : 当前脚本所处的是在子网1还是子网2（注意，该参数非常重要）
   ACTIVE : 当前子网是否为激活状态。如果取false，则在split后，当前子网的集群为只读状态。

2、修改完配置后，请将 cluster_opr.js、init.sh、merge.sh、split.sh 分别拷贝到 子网1 和子网2 的一台Catalog所在机器上（ 放置在 SequoiaDB 的安装目录下，默认 /opt/sequoiadb ）

3、分别在上述子网1和子网2的机器上的shell下执行 ' sh init.sh '，进行初始化（该初始化主要是保存当前集群所有的组信息，用于merge时恢复集群）

4、当子网1和子网2出现了网络分离，相互无法访问时，此时可以分别在上述子网1和子网2的机器上的shell下执行 ' sh split.sh ' 进行集群分离， 让子网1和子网2分离成独立集群，此时子网1可以对外提供读写操作，子网2可以提供读操作；

5、当子网1和子网2网络连通时，可以分别在上述子网1和子网2的机器上的shell下执行 ' sh merge.sh ' 进行集群合并， 把子网1和子网2独立的集群重新合成一个大群集。

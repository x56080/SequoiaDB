本节介绍使用 C 程序运行 SequoiaDB。首先安装 SequoiaDB，安装信息请查看 SequoiaDB 服务器安装章节。

这里介绍如何使用 C 客户端驱动接口编写使用 SequoiaDB 数据库的程序。为了简单起见，下面的例子不全部是完整的代码，只起示例性作用。可到 /sequoiadb/client/samples/C 下获取相应的完整的代码。

##数据库操作##

* 数据库连接（Connecting）编写完整客户端文件 connect.c 演示连接到数据库。文件必须包含“client.h”头文件。

  ```lang-c
  #include &lt;stdio.h>  
  #include "client.h"

  // Display Syntax Error
  void displaySyntax ( CHAR *pCommand )
  {
        printf ( "Syntax: %s&lt;hostname> &lt;servicename> &lt;username> &lt;password>"
        OSS_NEWLINE, pCommand ) ;
  }

  INT32 main ( INT32 argc, CHAR **argv )
  {
       // define a connecion handle; use to connect to database
       sdbConnectionHandle connection    = 0 ;
       INT32 rc = SDB_OK ;

       // verify syntax
       if ( 5 != argc )
       {
          displaySyntax ( (CHAR*)argv[0] ) ;
          exit ( 0 ) ;
       }

       // read argument
       CHAR *pHostName    = (CHAR*)argv[1] ;
       CHAR *pServiceName = (CHAR*)argv[2] ;
       CHAR *pUsr         = (CHAR*)argv[3] ;
       CHAR *pPasswd      = (CHAR*)argv[4] ;

       // connect to database
       rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
       if( rc!=SDB_OK )
       {
          printf("Fail to connet to database, rc = %d" OSS_NEWLINE, rc ) ;
          goto error ;
       }

       done:
       // disconnect from database
       sdbDisconnect ( connection ) ;
       // release connection
       sdbReleaseConnection ( connection ) ;
       return 0 ;
       error:
       goto done ;
  }
  ```

  在 Linux 下，可以如下编译及链接动态链接库文件 libsdbc.so。

  ```lang-bash
  $ gcc -o connect connect.c -I /< PATH >/sdbdriver/include -lsdbc -L /< PATH >/sdbdriver/lib
  $ ./connect localhost 11810 "" ""
  connect success!
  ```

  >**Note:**

  >本例程连接到本地数据库的11810端口，使用的是空的用户名很密码。用户需要根据自己的实际情况配置参数。但如果数据库已经创建用户，可以使用已经创建的用户及密码连接到数据库。

* 创建集合空间，集合

  以下创建了一个名字为“foo”的集合空间和一个名字为“bar”的集合，集合空间内的集合的数据页大小为4k。可根据实际情况选择不同大小的数据页。创建集合后，可对集合做增删改查等操作。

  ```lang-c
  // 首先，定义集合空间、集合句柄。
  sdbCSHandle collectionspace       = 0 ;
  sdbConnectionHandle connection    = 0 ;
  // 创建集合空间"foo"
  rc = sdbCreateCollectionSpace ( connection, "foo", SDB_PAGESIZE_4K, &collectionspace ) ;  
  // 在新建立的集合空间中创建集合"bar"
  rc = sdbCreateCollection ( collectionspace, "bar", &collection ) ;
  ```

  >**Note:**

  >在创建集合“bar”时并没有附加分区、压缩等信息，关于创建集合的更详细情况，请参考详情请查阅 [C API](api/c/html/index.html)

* 插入数据

  SequoiaDB 存储数据采用 BSON 的格式，BSON 是一种类似 JSON 的二进制对象。保存数据库中的数据，首先必须创建 bson 对象。下面会将{name:"Tom",age:24}插入到集合中。

  ```lang-c
  // 首先，我们需要创建一个插入的 bson 对象。
  INT32 rc = SDB_OK ;
  bson obj ;
  bson_init( &obj ) ;
  bson_append_string( &obj, "name", "tom" ) ;
  bson_append_int( &obj, "age", 24 ) ;
  rc = bson_finish( &obj ) ;
  if ( rc != SDB_OK )
  printf("Error.");
  // 接着，把此 bson 对象插入集合中
  rc = sdbInsert ( collection, &obj ) ;
  ```

* 查询

  查询操作需要一个游标句柄存放查询的结果到本地。要获得查询的结果需要使用游标操作。本例使用了游标操作的 sdbNext 接口，表示从查询结果中取到一条记录。此示例中没有设置查询条件，筛选条件，排序情况，及仅使用默认索引。

  ```lang-c
  // 定义一个游标句柄
  sdbCursorHandle cursor = 0 ;
  ...
  // 查询所有记录，查询结果放在游标句柄中
  rc = sdbQuery(collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
  // 从游标中显示所有记录
  bson_init( &obj );
  while( !( rc=sdbNext( cursor, &obj ) ) )
  {
       bson_print( &obj ) ;
       bson_destroy( &obj ) ;
       bson_init( &obj );
  }
  bson_destroy( &obj ) ;
  ```

* 索引

  此处，我们在集合句柄 collection 指定的集合中创建一个以“name”为升序，“age”为降序的索引。

  ```lang-c
  #define INDEX_NAME "index"
  ...
  // 首先创建一 bson 对象包含将要创建的索引的信息
  bson_init( &obj ) ;
  bson_append_int( &obj, "name", 1 ) ;
  bson_append_int( &obj, "age", -1 ) ;
  rc = bson_finish( &obj ) ;
  if ( rc != SDB_OK )
  printf("Error.");
  // 创建一个以"name"为升序，"age"为降序的索引
  rc = sdbCreateIndex ( collection, &obj, INDEX_NAME, FALSE, FALSE ) ;
  bson_destroy ( &obj ) ;
  ```

* 更新

  此处，我们在集合句柄 collection 指定的集合中更新记录。因为没有指定数据匹配规则，所以此示例将更新集合中所有的集合。

  ```lang-c
  // 先创建一个包含更新规则的 bson 对象
  bson_init( &rule ) ;
  bson_append_start_object ( &rule, "$set" ) ;
  bson_append_int ( &rule, "age", 19 ) ;
  bson_append_finish_object ( &rule ) ;
  rc = bson_finish ( &rule ) ;
  if ( rc != SDB_OK )
  printf("Error.");
  // 打印出更新规则
  bson_print( &rule ) ;
  // 更新记录
  rc = sdbUpdate( collection, &rule, NULL, NULL ) ;
  bson_destroy(&rule);
  ```

  此处，因为没有指定记录匹配条件，所以此示例将更新集合句柄 collection 指定的集合中所有的记录。

##集群操作##

* 分区组操作

  分区组操作包括创建分区组（sdbCreateReplicaGroup），得到分区组句柄（sdbGetReplicaGroup），启动分区组（sdbStartReplicaGroup），停止分区组（sdbStopReplicaGroup）等。以下为分区组操作示例性的例子。真正的应用应包括错误检测等。

  ```lang-c
  // 定义一个分区组句柄
  sdbReplicaGroupHandle rg = 0 ;
  ...
  // 先建立一个编目分区组
  rc = sdbCreateCataReplicaGroup ( connection, HOST_NAME, SERVICE_NAME, CATALOG_SET_PATH , NULL ) ;
  // 创建数据分区组
  rc = sdbCreateReplicaGroup ( connection, GROUP_NAME, &rg ) ;
  // 创建数据节点
  rc = sdbCreateNode ( rg, HOST_NAME1, SERVICE_NAME1, DATABASE_PATH1, NULL ) ;
  // 启动分区组
  rc = sdbStartReplicaGroup( rg ) ;
  ```

* 数据节点操作

  数据节点操作包括创建数据节点（sdbCreateNode），得到主数据节点（sdbGetNodeMaster），得到从数据节点（sdbGetNodeSlave），启动数据节点（sdbStartNode），停止数据节点（sdbStopNode）等。以下为数据节点操作示例性的例子。真正的应用应包括错误检测等。

  ```lang-c
  // 定义一个数据节点句柄
  sdbNodeHandle masternode   = 0 ;
  sdbNodeHandle slavenode    = 0 ;
  ...
  // 获取主数据节点
  rc = sdbGetNodeMaster ( rg, &masternode ) ;
  //获取从数据节点
  rc = sdbGetNodeSlave ( rg, &slavenode ) ;
  ```

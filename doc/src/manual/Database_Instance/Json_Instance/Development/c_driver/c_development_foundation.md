
本文档主要介绍如何使用 C 程序运行 SequoiaDB 巨杉数据库。运行前需要先部署 SequoiaDB，可参考[快速部署][quickstart]章节。

主要介绍内容为使用 C 客户端驱动接口编写使用 SequoiaDB 数据库的程序。下述为 SequoiaDB 巨杉数据库 C 驱动的简单示例，示例中的代码可能不完整，用户可在 `/sequoiadb/samples/C` 目录下获取相应的完整代码。

##数据库操作##

* 连接数据库

   通过编写完整客户端文件 `connect.c` 连接到数据库，文件必须包含“client.h”头文件

   ```lang-c
   #include <stdio.h>  
   #include "client.h"
   
   // Display Syntax Error
   void displaySyntax ( CHAR *pCommand )
   {
         printf ( "Syntax: %s<hostname> <servicename> <username> <password>"
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
   

   在 Linux 下，可以进行如下编译并链接动态链接库文件 `libsdbc.so`：

   ```lang-bash
   $ gcc -o connect connect.c -I /< PATH >/sdbdriver/include -lsdbc -L /< PATH >/sdbdriver/lib
   $ ./connect localhost 11810 "" ""
   ```

   >**Note:**
   >
   >本示例连接到本地数据库的 11810 端口，使用的是空的用户名很密码，用户可以根据实际情况配置参数。如果数据库已经创建用户，可以使用已经创建的用户及密码连接到数据库。

* 创建集合空间和集合

   ```lang-c
   // 定义集合空间和集合句柄
   sdbCSHandle collectionspace       = 0 ;
   sdbConnectionHandle connection    = 0 ;
   // 创建集合空间 sample，配置集合空间内的集合的数据页大小为 4k
   rc = sdbCreateCollectionSpace ( connection, "sample", SDB_PAGESIZE_4K, &collectionspace ) ;  
   // 在新建立的集合空间中创建集合 employee
   rc = sdbCreateCollection ( collectionspace, "employee", &collection ) ;
   ```

   用户创建集合后，可对集合进行增删改查等操作。

   >**Note:**
   >
   > 上述示例中，在创建集合 employee 时并没有附加分区、压缩等信息，如果需要配置可参考 [C API][api]。

* 插入数据

   SequoiaDB 存储数据采用 BSON 的格式，BSON 是一种类似 JSON 的二进制对象；保存数据库中的数据，必须先创建 bson 对象，以下示例会将 {name:"Tom",age:24} 插入到集合中

   ```lang-c
   // 创建包含插入信息的 bson 对象
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   bson_append_string( &obj, "name", "tom" ) ;
   bson_append_int( &obj, "age", 24 ) ;
   rc = bson_finish( &obj ) ;
   if ( rc != SDB_OK )
   printf("Error.");
   // 将此 bson 对象插入集合中
   rc = sdbInsert ( collection, &obj ) ;
   ```

* 查询

   查询操作需要一个游标句柄存放查询的结果到本地；以下示例使用游标操作的 sdbNext 接口，表示从查询结果中取到一条记录

   ```lang-c
   // 定义游标句柄
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

   在集合句柄 collection 指定的集合中创建一个以“name”为升序、“age”为降序的索引

   ```lang-c
   #define INDEX_NAME "index"
   ...
   // 创建包含指定索引信息的 bson 对象
   bson_init( &obj ) ;
   bson_append_int( &obj, "name", 1 ) ;
   bson_append_int( &obj, "age", -1 ) ;
   rc = bson_finish( &obj ) ;
   if ( rc != SDB_OK )
   printf("Error.");
   // 创建以"name"为升序，"age"为降序的索引
   rc = sdbCreateIndex ( collection, &obj, INDEX_NAME, FALSE, FALSE ) ;
   bson_destroy ( &obj ) ;
   ```

* 更新

   在集合句柄 collection 指定的集合中更新记录

   ```lang-c
   // 创建包含更新规则的 bson 对象
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

   示例中没有指定记录匹配条件，所以将更新集合句柄 collection 中指定集合的所有记录。

##集群操作##

* 复制组操作

   复制组操作包括创建复制组（sdbCreateReplicaGroup）、得到复制组句柄（sdbGetReplicaGroup）、启动复制组（sdbStartReplicaGroup）、停止复制组（sdbStopReplicaGroup）等。以下为复制组操作示例性的例子，真正的应用需要包括错误检测等操作。

   ```lang-c
   // 定义复制组句柄
   sdbReplicaGroupHandle rg = 0 ;
   ...
   // 建立编目复制组
   rc = sdbCreateCataReplicaGroup ( connection, HOST_NAME, SERVICE_NAME, CATALOG_SET_PATH , NULL ) ;
   // 创建数据复制组
   rc = sdbCreateReplicaGroup ( connection, GROUP_NAME, &rg ) ;
   // 创建数据节点
   rc = sdbCreateNode ( rg, HOST_NAME1, SERVICE_NAME1, DATABASE_PATH1, NULL ) ;
   // 启动复制组
   rc = sdbStartReplicaGroup( rg ) ;
   ```

* 数据节点操作

   数据节点操作包括创建数据节点（sdbCreateNode）、得到主数据节点（sdbGetNodeMaster），得到从数据节点（sdbGetNodeSlave）、启动数据节点（sdbStartNode）、停止数据节点（sdbStopNode）等。以下为数据节点操作示例性的例子，真正的应用需要包括错误检测等。

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

[^_^]:
     本文使用的所有引用和链接
[quickstart]:manual/Quick_Start/quick_deployment.md
[api]:manual/Database_Instance/Json_Instance/Development/c_driver/c_api.md
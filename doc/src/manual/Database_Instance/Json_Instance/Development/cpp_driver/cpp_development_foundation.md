本文档主要介绍如何使用 C++ 客户端驱动接口编写使用 SequoiaDB 巨杉数据库的程序。下述为 SequoiaDB 巨杉数据库 C++ 驱动的简单示例，示例中的代码可能不完整，用户可在 `/sequoiadb/samples/CPP` 目录下获取相应的完整代码。

##数据库操作##

* 连接数据库

   通过编写完整客户端文件 `connect.cpp` 连接到数据库，文件应当包含“client.hpp”头文件及使用命名空间 sdbclient

   ```lang-cpp
   #include <iostream>
   #include "client.hpp"
 
   using namespace std ;
   using namespace sdbclient ;
 
   // Display Syntax Error
   void displaySyntax ( CHAR *pCommand ) ;
 
   INT32 main ( INT32 argc, CHAR **argv )
   {
        // verify syntax
        if ( 5 != argc )
        {
           displaySyntax ( (CHAR*)argv[0] ) ;
           exit ( 0 ) ;
        }
        // read argument
        CHAR *pHostName    = (CHAR*)argv[1] ;
        CHAR *pPort        = (CHAR*)argv[2] ;
        CHAR *pUsr         = (CHAR*)argv[3] ;
        CHAR *pPasswd      = (CHAR*)argv[4] ;
 
        // define local variable
        sdb connection ;
        INT32 rc = SDB_OK ;
 
        // connect to database
        rc = connection.connect ( pHostName, pPort, pUsr, pPasswd ) ;
        if( rc!=SDB_OK )
        {
           cout << "Fail to connet to database, rc = " << rc << endl ;
           goto error ;
        }
        else
        cout << "Connect success!" << endl ;
 
       done:
          // disconnect from database
          connection.disconnect () ;
          return 0 ;
       error:
          goto done ;
     }
 
     // Display Syntax Error
     void displaySyntax ( CHAR *pCommand )
     {
       cout << "Syntax:" << pCommand << " <hostname>  <servicename>  <username>  <password> " << endl ;
     }
   ```

   在 Linux 下，可以使用如下命令编译及链接动态链接库文件 `libsdbcpp.so`：

  ```lang-bash
  $ g++ -o connect connect.cpp -I <PATH>/sdbdriver/include -lsdbcpp -L <PATH>/sdbdriver/lib  
  $ ./connect localhost 11810 "" ""
  ```

  >**Note:**
  >
  > * 当用户使用的 GCC 编译器版本大于 GCC 4.x（如 GCC 5 或以上版本）时，使用 CPP 驱动动态库或者静态库需要添加 -D_GLIBCXX_USE_CXX11_ABI=0 编译选项。
  > * 本示例连接到本地数据库的 11810 端口，使用的是空的用户名和密码。用户可以实际情况配置参数，如：`./connect localhost 11810 "sequoiadb" "sequoiadb"`。当数据库已经创建用户时，应该使用正确的用户及密码连接到数据库，否则连接失败。

* 创建集合空间和集合

   ```lang-cpp
   // 定义集合空间和集合对象
   sdbCollectionSpace collectionspace ;
   sdbCollection collection ;
   // 创建集合空间 sample，配置集合空间内的集合的数据页大小为 4k
   rc = connection.createCollectionSpace ( "sample", SDB_PAGESIZE_4K, collectionspace ) ;
   // 在新建立的集合空间中创建集合 employee
   rc = collectionspace.createCollection ( "employee", collection ) ;
   ```

   用户创建集合后，可对集合进行增删改查等操作。

   >**Note:**
   >
   > 上述示例中，在创建集合 employee 时并没有附加分区、压缩等信息，如果需要配置可参考 [C++ API](api/cpp/html/index.html)。

* 插入数据


   ```lang-cpp
   // 创建包含插入信息的 bson 对象
   BSONObj obj ;
   obj = BSON ( "name" << "tom" << "age" << 24 ) ;
   // 将此 bson 对象插入集合中
   collection.insert ( obj ) ;
   ```

* 查询

 查询操作需要一个游标对象存放查询的结果到本地；以下示例使用了游标操作的 next 接口，表示从查询结果中取到一条记录： 

   ```lang-cpp
   // 定义一个游标对象
   sdbCursor cursor ;
   // 查询所有记录，并把查询结果放在游标对象中
   collection.query ( cursor ) ;
   // 从游标中显示所有记录
   while( !( rc=cursor.next( obj ) ) )
   {
       cout << obj.toString() << endl ;
   }
   ```

* 索引

 在集合对象 collection 中创建一个以"name"为升序、"age"为降序的索引

   ```lang-cpp
   # define INDEX_NAME "index"
   // 创建包含指定索引信息的 BSONObj 对象
   BSONObj obj ;
   obj = BSON ( "name" << 1 << "age" << -1 ) ;
   // 创建一个以"name"为升序，"age"为降序的索引
   collection.createIndex ( obj, INDEX_NAME, FALSE, FALSE ) ;
   ```

* 更新

 在集合对象 collection 中更新记录

   ```lang-cpp
   // 创建包含更新规则的 BSONObj 对象
   BSONObj rule = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   // 打印出更新规则
   cout << rule.toString() << endl ;
   // 更新记录
   collection.update( rule ) ;
   ```

   示例中没有指定数据匹配条件，所以将更新集合中所有的记录。

##集群操作##

* 复制组操作

  复制组操作包括创建复制组（sdb:createReplicaGroup）、得到复制组实例（sdb:getReplicaGroup）、启动复制组所有数据节点（sdbReplicaGroup::start）、停止复制组所有数据节点（sdbReplicaGroup::stop）等。以下为复制组操作示例性的例子，真正的应用需要包括错误检测等。

   ```lang-cpp
   // 定义一个复制组实例
   sdbReplicaGroup rg  ;
   // 定义创建节点需要使用的配置项，此处定义一个空的配置项，表示使用默认配置
   BSONObj conf ;
   // 建立编目复制组
   connection.createCataReplicaGroup ( "ubuntu-dev1", "30000", "/opt/sequoiadb/database/catalog/30000", conf ) ;
   // 创建数据复制组
   connection.createRG ( "dataGroup1", rg ) ;
   // 创建第一个数据节点
   rg.createNode ( "ubuntu-dev1", "40000", "/opt/sequoiadb/database/data/40000", conf ) ;
   // 启动复制组
   rg.start () ;
  ```

* 数据节点操作

  数据节点操作包括创建数据节点（sdbReplicaGroup::createNode）、得到主数据节点（sdbReplicaGroup::getMaster）、得到从数据节点（sdbReplicaGroup::getSlave）、启动数据节点（sdbNode::start）、停止数据节点（sdbNode::stop）等。以下为数据节点操作示例性的例子，真正的应用需要包括错误检测等。

   ```lang-cpp
   // 定义一个数据节点实例  
   sdbNode masternode ;
   sdbNode slavenode ;
   // 获取主数据节点
   rg.getMaster( masternode ) ;
   // 获取从数据节点
   rg.getSlave( slavenode ) ;
   ```

本文档主要介绍使用 C++ 驱动的操作流程，以及通过 [C++ 客户端驱动接口][api]编写使用 SequoiaDB 巨杉数据库的程序。完整的示例代码可参考 SequoiaDB 安装目录下的 `samples/CPP`。

##操作流程##

下述以操作协调节点为 11810、用户名为“sdbadmin”、密码为“sdbadmin”的本地数据库为例，介绍使用 C++ 驱动的操作流程。

1. 编写操作 SequoiaDB 的代码 `connect.cpp`，用户可参考[代码示例][example]

2. 编译 `connect.cpp` 并链接库文件，用户可选择的链接方式如下：

    使用动态库 `libsdbcpp.so` 进行编译

    ```lang-bash
    $ g++ connect.cpp -o connect -I <PATH>/sdbdriver/include -L <PATH>/sdbdriver/lib -lsdbcpp
    ```

    使用静态库 `libstaticsdbcpp.a` 进行编译
   
    ```lang-bash
    $ g++ connect.cpp -o connect -I <PATH>/sdbdriver/include -L <PATH>/sdbdriver/lib -lstaticsdbcpp -lm -lpthread -ldl
    ```

    >**Note:**
    >
    > - \<PATH\> 为驱动包的放置路径。
    > - 当用户使用的 GCC 编译器版本小于 GCC 5.1 时，使用 CPP 驱动动态库或者静态库不需要添加 -D_GLIBCXX_USE_CXX11_ABI=0 编译选项。

3. 执行 `connect`，并连接本地数据库

    ```lang-bash
    $ ./connect localhost 11810 "sdbadmin" "sdbadmin"
    ```

##代码示例##

###连接数据库###

* 连接 SequoiaDB 数据库，之后即可访问、操作数据库。如下为连接数据库的示例代码 `connect.cpp`。代码中需要包含 “client.hpp” 头文件及使用命名空间 sdbclient

   ```lang-cpp
   #include <iostream>
   #include "client.hpp"
 
   using namespace std ;
   using namespace sdbclient ;
   using namespace bson ;
 
   // Display Syntax Error
   void displaySyntax ( CHAR *pCommand ) ;
 
   INT32 main ( INT32 argc, CHAR **argv )
   {
        // verify syntax
        if ( 5 != argc )
        {
           displaySyntax( (CHAR*)argv[0] ) ;
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
        rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
        if( rc!=SDB_OK )
        {
           cout << "Fail to connet to database, rc = " << rc << endl ;
           goto error ;
        }
        else
        cout << "Connect success!" << endl ;
 
       done:
          // disconnect from database
          connection.disconnect() ;
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

###数据库操作###

* 创建集合空间和集合

   ```lang-cpp
   // 定义集合空间和集合对象
   sdbCollectionSpace collectionspace ;
   sdbCollection collection ;
   // 创建集合空间 sample，配置集合空间内的集合的数据页大小为 4k
   rc = connection.createCollectionSpace( "sample", SDB_PAGESIZE_4K, collectionspace ) ;
   // 在新建立的集合空间中创建集合 employee
   rc = collectionspace.createCollection( "employee", collection ) ;
   ```

* 插入数据

   ```lang-cpp
   BSONObj obj ;
   BSONObj result ;
   // 准备需要插入的数据
   obj = BSON( "name" << "tom" << "age" << 24 ) ;
   // 插入
   collection.insert( obj, FLG_INSERT_RETURN_OID, &result ) ;
   // 查看插入的结果
   cout<< "Insert result: " << result.toString() <<endl ;
   ```

* 查询

   ```lang-cpp
   // 定义一个游标对象
   sdbCursor cursor ;
   // 查询所有记录，并把查询结果放在游标对象中
   collection.query( cursor ) ;
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

   ```lang-cpp
   BSONObj modifier ;
   BSONObj matcher ;
   BSONObj hint ;
   BSONObj result ;
   // 设置更新的内容
   modifier = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout << modifier.toString() << endl ;
   // 更新集合中的所有记录
   collection.update( modifier, matcher, hint, 0, &result ) ;
   // 查看更新的结果
   cout<< "Update result: " << result.toString() <<endl ;
   ```

* 删除

   ```lang-cpp
   BSONObj matcher ;
   BSONObj hint ;
   BSONObj result ;
   // 设置删除条件
   matcher = BSON( "age" << 19 ) ;
   // 更新集合中的所有记录
   collection.del( matcher, hint, 0, &result ) ;
   // 查看删除的结果
   cout<< "Delete result: " << result.toString() <<endl ;
   ```

###集群操作###

集群操作主要涉及复制组与节点。如下以创建复制组、获取节点为例

* 创建复制组

   ```lang-cpp
   // 定义一个复制组实例
   sdbReplicaGroup rg  ;
   // 定义创建节点需要使用的配置项，此处定义一个空的配置项，表示使用默认配置
   BSONObj conf ;
   // 创建数据复制组
   connection.createRG ( "dataGroup", rg ) ;
   // 创建第一个数据节点
   rg.createNode ( "sdbserver", "11820", "/opt/sequoiadb/database/data/11820", conf ) ;
   // 启动复制组
   rg.start () ;
  ```

* 获取节点

   ```lang-cpp
   // 定义数据节点实例
   sdbNode node ;
   // 获取主数据节点
   rg.getMaster( node ) ;
   ```

[^_^]:
     本文使用的所有引用和链接
[api]:manual/Database_Instance/Json_Instance/Development/cpp_driver/cpp_api.md
[example]:manual/Database_Instance/Json_Instance/Development/cpp_driver/cpp_development_foundation.md#代码示例
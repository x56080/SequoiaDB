这里介绍如何使用 C++ 客户端驱动接口编写使用 SequoiaDB 数据库的程序。为了简单起见，下面的示例不全部是完整的代码，只起示例性作用。可到 /sequoiadb/client/samples/CPP 下获取相应的完整的代码。更多查看 [C++ API](api/cpp/html/index.html)

##数据库操作##

* 连接数据库：connect.cpp 演示如何连接到数据库。文件应当包含“client.hpp”头文件及使用命名空间 sdbclient。

  ```lang-javascript
  #include &lt;iostream&gt;
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
      cout << "Syntax:" << pCommand << " &lt;hostname&gt;  &lt;servicename&gt;  &lt;username&gt;  &lt;password&gt; " << endl ;
    }
  ```

  在 Linux下，可以如下编译及链接动态链接库文件 libsdbcpp.so:

  ```lang-javascript
  $ g++ -o connect connect.cpp -I &lt;PATH&gt;/sdbdriver/include -lsdbcpp -L &lt;PATH&gt;/sdbdriver/lib  
  执行结果如下：
  $ ./connect localhost 11810 "" ""
  Connect success!
  ```

  >**Note:**
  >
  >本例程连接到本地数据库的11810端口，使用的是空的用户名和密码。用户需要根据自己的实际情况配置参数。譬如：`./connect localhost 11810 "sequoiadb" "sequoiadb"`。当数据库已经创建用户时，应该使用正确的用户及密码连接到数据库，否则连接失败。

* 创建集合空间和集合


  首先，定义集合空间，集合对象。

  ```lang-javascript
  sdbCollectionSpace collectionspace ;
  sdbCollection collection ;
  ```
  
  创建集合空间"foo"
  
  ```lang-javascript
  rc = connection.createCollectionSpace ( "foo", SDB_PAGESIZE_4K, collectionspace ) ;
  ```

  在新建立的集合空间中创建集合"bar"

  ```lang-javascript
  rc = collectionspace.createCollection ( "bar", collection ) ;
  ```

  以上创建了一个名字为“foo”的集合空间和一个名字为“bar”的集合，集合空间内的集合的数据页大小为4k。可根据实际情况选择不同大小的数据页。创建集合后，可对集合做增删改查等操作。

  >**Note:**
  >
  >在创建集合“bar”时并没有附加分区，压缩等信息，详情请查阅 [C++ API](api/cpp/html/index.html)

* 插入数据：insert

  首先，需要创建一个插入的 bson 对象。

  ```lang-javascript
  BSONObj obj ;
  obj = BSON ( "name" << "tom" << "age" << 24 ) ;
  ```

  接着，把此 bson 对象插入集合中
 
  ```lang-javascript
  collection.insert ( obj ) ;
  ```

  obj 为输入参数，为要插入的数据。

* 查询：query

  定义一个游标对象

  ```lang-javascript
  sdbCursor cursor ;
  ```

  查询所有记录，并把查询结果放在游标对象中

  ```lang-javascript
  collection.query ( cursor ) ;
  ```

  从游标中显示所有记录

  ```lang-javascript
  while( !( rc=cursor.next( obj ) ) )
  {
      cout << obj.toString() << endl ;
  }
  ```

  查询操作需要一个游标对象存放查询的结果到本地。要获得查询的结果需要使用游标操作。本例使用了游标操作的 next 接口，表示从查询结果中取到一条记录。此示例中没有设置查询条件，筛选条件，排序情况，及仅使用默认索引。

* 创建索引：index

  ```lang-javascript
  # define INDEX_NAME "index"
  ```

  首先创建一 BSONObj 对象包含将要创建的索引的信息

  ```lang-javascript
  BSONObj obj ;
  obj = BSON ( "name" << 1 << "age" << -1 ) ;
  ```

  创建一个以"name"为升序，"age"为降序的索引

  ```lang-javascript
  collection.createIndex ( obj, INDEX_NAME, FALSE, FALSE ) ;
  ```

  集合对象 collection 中创建一个以"name"为升序，"age"为降序的索引。

* 更新：update

  先创建一个包含更新规则的 BSONObj 对象

  ```lang-javascript
  BSONObj rule = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
  ```

  打印出更新规则

  ```lang-javascript
  cout << rule.toString() << endl ;
  ```

  更新记录

  ```lang-javascript
  collection.update( rule ) ;
  ```

  在集合对象 collection 中更新了记录。示例中没有指定数据匹配规则，所以此示例将更新集合中所有的记录。

##集群操作##

* 分区组操作

  分区组操作包括创建分区组（sdb:createReplicaGroup），得到分区组实例（sdb:getReplicaGroup），启动分区组所有数据节点（sdbReplicaGroup::start），停止分区组所有数据节点（sdbReplicaGroup::stop）等。

  以下为分区组操作示例性的例子。真正的应用应包括错误检测等。

  定义一个分区组实例

  ```lang-javascript
  sdbReplicaGroup rg  ;
  ```

  定义创建节点需要使用的配置项，此处定义一个空的配置项，表示使用默认配置

  ```lang-javascript
  BSONObj conf ;
  ```

  先建立一个编目分区组

  ```lang-javascript
  connection.createCataReplicaGroup ( "ubuntu-dev1", "30000", "/opt/sequoiadb/database/catalog/30000", conf ) ;
  ```

  创建数据分区组

  ```lang-javascript
  connection.createRG ( "dataGroup1", rg ) ;
  ```

  创建第一个数据节点

  ```lang-javascript
  rg.createNode ( "ubuntu-dev1", "40000", "/opt/sequoiadb/database/data/40000", conf ) ;
  ```

  启动分区组

  ```lang-javascript
  rg.start () ;
  ```

* 数据节点操作

  数据节点操作包括创建数据节点（sdbReplicaGroup::createNode），得到主数据节点（sdbReplicaGroup::getMaster），得到从数据节点（sdbReplicaGroup::getSlave），启动数据节点（sdbNode::start），停止数据节点（sdbNode::stop）等。

  以下为数据节点操作示例性的例子。真正的应用应包括错误检测等。

  定义一个数据节点实例

  ```lang-javascript
  sdbNode masternode ;
  sdbNode slavenode ;
  ```

  获取主数据节点

  ```lang-javascript
  rg.getMaster( masternode ) ;
  ```

  获取从数据节点

  ```lang-javascript
  rg.getSlave( slavenode ) ;
  ```

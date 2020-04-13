这里介绍如何使用 CSharp 驱动接口编写使用 SequoiaDB 数据库的程序。该文档介绍了 SequoiaDB 数据库 CSharp 驱动的简单示例，详细的使用规范可参照官方的 [CSharp API](api/cs/html/index.html) 文档。

##命名空间##

在使用 CSharp 驱动的相关 API 之前，你必须在源代码中添加如下的 using 申明：

```lang-c#
using SequoiaDB;
using SequoiaDB.Bson;
```

数据操作

* 连接数据库和身份验证

  若数据库没有创建用户，则可以匿名连接到数据库：

  ```lang-c#
  string addr = "127.0.0.1:11810";
  Sequoiadb sdb = new Sequoiadb(addr);
  try
  {
        sdb.Connect();
  }
  catch (BaseException e)
  {
        Console.WriteLine("ErrorCode:{0}, ErrorType:{1}", e.ErrorCode, e.ErrorType);
        Console.WriteLine(e.Message);
  }
  catch (System.Exception e)
  {
        Console.WriteLine(e.StackTrace);
  }
  ```

  否则，连接的时候必须指定用户名和密码：

  ```lang-c#
  string addr = "127.0.0.1:11810";
  Sequoiadb sdb = new Sequoiadb(addr);
  try
  {
        sdb.Connect("testusr", "testpwd");
  }
  catch (BaseException e)
  {
        Console.WriteLine("ErrorCode:{0}, ErrorType:{1}", e.ErrorCode, e.ErrorType);
        Console.WriteLine(e.Message);
  }
  catch (System.Exception e)
  {
        Console.WriteLine(e.StackTrace);
  }
  ```

  这里给出了异常信息的 try 和 catch 块，下面的所有操作都会抛出同样的异常信息，因此不再给出相关的 try 和 catch 块。

* 断开与数据库连接

  ```lang-c#
  // do not forget to disconnect from sdb
  sdb.Disconnect();
  ```

* 得到或创建集合空间和集合

  根据名字，得到对应的 CollectionSpace，如果不存在，则创建：

  ```lang-c#
  // get if the CollectionSpace exists, otherwise create
  string csName = "TestCS";
  CollectionSpace cs = null;
  if (sdb.IsCollectionSpaceExist(csName))
  {
        cs = sdb.GetCollectionSpace(csName);
  }
  else
  {
        cs = sdb.CreateCollectionSpace(csName);
        // or sdb.CreateCollectionSpace(csName, pageSize), need to specify the pageSize
  }
  ```

  根据名字，得到对应的 Collection，如果不存在，则创建：

  ```lang-c#
  // get if the Collection exists, otherwise create
  string clName = "TestCL";
  DBCollection dbc = null;
  if (cs.IsCollectionExist(clName))
  {
        dbc = cs.GetCollection(clName);
  }
  else
  {
        dbc = cs.CreateCollection(clName);
        //or cs.CreateCollection(clName, options), create collection with some options
  }
  ```

* 对 Collection 进行插入操作

  创建需要插入的数据 BsonDocument 并插入：

  ```lang-c#
  BsonDocument insertor = new BsonDocument();
  string date = DateTime.Now.ToString();
  insertor.Add("operation", "Insert");
  insertor.Add("date", date);
  dbc.Insert(insertor);
  ```

  当然，BsonDocument 中还可以嵌套 BsonDocument 对象；而且你还可以直接 new 一个完整的 BsonDocument，而不需要通过 Add 方法：

  ```lang-c#
  BsonDocumentinsertor = new BsonDocument
  {
       {"FirstName","John"},
       {"LastName","Smith"},
       {"Age",50},
       {"id",i},
       {"Address",
           new BsonDocument
           {
              {"StreetAddress","212ndStreet"},
              {"City","NewYork"},
              {"State","NY"},
              {"PostalCode","10021"}
           }
       },
       {"PhoneNumber",
            new BsonDocument
            {
               {"Type","Home"},
               {"Number","212555-1234"}
          }
       }
  };
  ```

  插入多条数据：

  ```lang-c#
  //bulkinsert
  List< BsonDocument > insertor=new List < BsonDocument > ();
  for(int i=0;i<10;i++)
  {
       BsonDocument obj=new BsonDocument();
       obj.Add("operation","BulkInsert");
       obj.Add("date",DateTime.Now.ToString());
       insertor.Add(obj);
  }
  dbc.BulkInsert(insertor,0);
  ```

* 索引的相关操作

  创建索引：

  ```lang-c#
  //createindexkey,indexonattribute'Id'byASC(1)/DESC(-1)
  BsonDocument key = new BsonDocument();
  key.Add("id", 1);
  string name = "index name";
  bool isUnique = true;
  bool isEnforced = true;
  dbc.CreateIndex(name, key, isUnique, isEnforced);
  ```

  删除索引：

  ```lang-c#
  string name = "index name";
  dbc.DropIndex(name);
  ```

* 查询操作

  进行查询操作，需要使用游标对查询结果进行遍历，而且可以先得到当前 Collection 的索引，如果不为空，可作为制定访问计划（hint）用于查询：

  ```lang-c#
  DBCursor icursor = dbc.GetIndex(name);
  BsonDocument index = icursor.Current();
  ```

  构建相应的 BsonDocument 对象用于查询，包括：查询匹配规则（matcher，包含相应的查询条件），域选择（selector），排序规则（orderBy，增序或降序），制定访问计划（hint），跳过记录个数（0），返回记录个数（-1：返回所有数据）。查询后，得到对应的 Cursor，用于遍历查询得到的结果：

  ```lang-c#
  BsonDocument matcher = new BsonDocument();
  BsonDocument conditon = new BsonDocument();
  conditon.Add("$gte", 0);
  conditon.Add("$lte", 9);
  matcher.Add("id", conditon);
  BsonDocument selector = new BsonDocument();
  selector.Add("id", null);
  selector.Add("Age", null);
  BsonDocument orderBy = new BsonDocument();
  orderBy.Add("id", -1);
  BsonDocument hint = null;
  if (index != null)
        hint = index;
  else
        hint = new BsonDocument();
  DBCursor cursor = dbc.Query(matcher, selector, orderBy, hint, 0, -1);
  ```

  使用 DBCursor 游标进行遍历：

  ```lang-c#
  while (cursor.Next() != null)
  Console.WriteLine(cursor.Current());
  ```

* 删除操作

  构建相应的 BsonDocument 对象，用于设置删除的条件：

  ```lang-c#
  //createthedeletecondition
  BsonDocument drop = new BsonDocument();
  drop.Add("Last Name", "Smith");
  coll.Delete(drop);
  ```

* 更新操作


  构建相应的 BsonDocument 对象，用于设置更新条件，你还可以创建 DBQuery 对象封装所有的查询或更新规则：

  ```lang-c#
  DBQuery query = new DBQuery();
  BsonDocument updater = new BsonDocument();
  BsonDocument matcher = new BsonDocument();
  BsonDocument modifier = new BsonDocument();
  updater.Add("Age", 25);
  modifier.Add("$set", updater);
  matcher.Add("First Name", "John");
  query.Matcher = matcher;
  query.Modifier = modifier;
  dbc.Update(query);
  ```

  更新操作，如果没有满足 matcher 的条件，则插入此记录：

  ```lang-c#
  dbc.Upsert(query);
  ```

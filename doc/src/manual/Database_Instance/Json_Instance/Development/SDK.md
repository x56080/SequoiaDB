[^_^]:
    SDK驱动开发
    作者：黎锐昌
    时间：20191211
    评审意见
    王涛：
    许建辉：
    市场部：


SequoiaDB 巨杉数据库为应用提供通过 SDK 驱动进行数据库操作和集群操作的接口。目前支持 SDK 驱动开发的语言如下：

+ C 驱动
+ C++ 驱动
+ CSharp 驱动
+ JAVA 驱动
+ PHP 驱动 
+ Python 驱动
+ REST 驱动

SDK驱动开发
----

下述以 JAVA SDK 驱动开发实现数据库增、删、改、查功能为例，使用 Eclipse 工具开发，数据库用户名和密码默认为 sdbadmin，192.168.81.134 是数据库的服务 IP，11810 端口是数据库协调节点的服务端口。

### 获取 JAVA 驱动开发包

用户可以从 [SequoiaDB 巨杉数据库官网](http://download.sequoiadb.com/cn/driver)下载对应操作系统版本的 JAVA 驱动开发包。

### 配置 Eclipse 开发环境

1. 在 Eclipse 界面中，创建或打开相应的开发工程

2. 解压驱动开发包，将开发包中的 `sequoiadb-driver.jar` 文件拷贝到工程文件目录下（建议将其放置在其他所有依赖库目录，如 `lib` 目录）

3. 在 Eclipse 主窗口左侧的【Project Explore】窗口中，选择开发工程，并点击鼠标右键

4. 在选项中依次选择【Build Path】->【Configure Build Path...】

5. 在弹出的【Properties for test】窗口中，进入【Libraries】页面点击 **Add JARs** 按钮

6. 在弹出的【JAR Selection】窗口中，选中对应开发工程中的 `sequoiadb-driver.jar` 文件

7. 点击 **OK** 按钮，完成环境配置

### 数据库操作

- 连接数据库

   ```lang-java
   package com.sequoiadb.util;
   
   import com.sequoiadb.base.DBCursor;
   import com.sequoiadb.base.Sequoiadb;
   import com.sequoiadb.exception.BaseException;
   
   public class Sample {
   	public static void main(String[] args) {
           String connString = "192.168.81.134:11810";
           try {
           	// 建立 SequoiaDB 数据库连接
           	Sequoiadb sdb = new Sequoiadb(connString, "sdbadmin", "sdbadmin");
               // 获取所有 Collection 信息，并打印出来
               DBCursor cursor = sdb.listCollections();
               try {
                   while(cursor.hasNext()) {
                       System.out.println(cursor.getNext());
                   }
               } finally {
                   cursor.close();
               }
           } catch (BaseException e) {
               System.out.println("Sequoiadb driver error, error description:" + e.getErrorType());
           }
       }
   }
   ```

- 插入数据

   ```lang-java
   package com.sequoiadb.util;
   
   import org.bson.BSONObject;
   import org.bson.BasicBSONObject;
   
   import com.sequoiadb.base.CollectionSpace;
   import com.sequoiadb.base.DBCollection;
   import com.sequoiadb.base.Sequoiadb;
   import com.sequoiadb.exception.BaseException;
   
   public class Sample {
   	public static void main(String[] args) {
           String connString = "192.168.81.134:11810";
           try {
               Sequoiadb sdb = new Sequoiadb(connString, "sdbadmin", "sdbadmin");
               // 创建 school 集合空间
               CollectionSpace cs = sdb.createCollectionSpace("school");
               // 创建 student 集合
               DBCollection cl = cs.createCollection("student");
               // 创建一个插入的 bson 对象
               BSONObject obj = new BasicBSONObject();
               obj.put("id", 1);
               obj.put("name", "tom");
               obj.put("age", 24);
               cl.insert(obj);
           } catch (BaseException e) {
               System.out.println("Sequoiadb driver error, error description:" + e.getErrorType());
           }
       }
   }
   ```

- 查询数据

   ```lang-java
   package com.sequoiadb.util;
   
   import org.bson.BSONObject;
   import org.bson.util.JSON;
   
   import com.sequoiadb.base.CollectionSpace;
   import com.sequoiadb.base.DBCollection;
   import com.sequoiadb.base.DBCursor;
   import com.sequoiadb.base.Sequoiadb;
   import com.sequoiadb.exception.BaseException;
   
   public class Sample {
   	public static void main(String[] args) {
           String connString = "192.168.81.134:11810";
           // 定义一个游标对象
           DBCursor cursor = null;
           try {
               Sequoiadb sdb = new Sequoiadb(connString, "sdbadmin", "sdbadmin");
               // 获取 school 集合空间对象
               CollectionSpace cs = sdb.getCollectionSpace("school");
               // 获取 student 集合对象
               DBCollection cl = cs.getCollection("student");
               // 查询 id 为 1 的记录
               BSONObject queryCondition = (BSONObject) JSON.parse("{id:1}");
               // 查询所有记录，并把查询结果放在游标对象中
               cursor = cl.query(queryCondition, null, null, null);
               // 从游标中显示所有记录
               try {
                   while (cursor.hasNext()) {
                      BSONObject record = cursor.getNext();
                      String name = (String) record.get("name");
                      int age = (int) record.get("age");
                      System.out.println("name = " +  name);
                      System.out.println("age = " +  age);
                   } 
               } finally {
               	if(cursor != null){
               		cursor.close();   
               	}
               }
           } catch (BaseException e) {
               System.out.println("Sequoiadb driver error, error description:" + e.getErrorType());
           }
       }
   }
   ```

- 修改数据

   ```lang-java
   package com.sequoiadb.util;
   
   import org.bson.BSONObject;
   import org.bson.util.JSON;
   
   import com.sequoiadb.base.CollectionSpace;
   import com.sequoiadb.base.DBCollection;
   import com.sequoiadb.base.Sequoiadb;
   import com.sequoiadb.exception.BaseException;
   
   public class Sample {
   	public static void main(String[] args) {
           String connString = "192.168.81.134:11810";
           try {
               Sequoiadb sdb = new Sequoiadb(connString, "sdbadmin", "sdbadmin");
               // 获取 school 集合空间对象
               CollectionSpace cs = sdb.getCollectionSpace("school");
               // 获取 student 集合对象
               DBCollection cl = cs.getCollection("student");
               // 修改 id 为 1 记录的 age 值为 30
               BSONObject queryCondition = (BSONObject) JSON.parse("{id:1}");
               BSONObject modifier = (BSONObject) JSON.parse("{$set:{age:30}}");
               cl.update(queryCondition, modifier, null);
           } catch (BaseException e) {
               System.out.println("Sequoiadb driver error, error description:" + e.getErrorType());
           }
       }
   }
   ```

- 删除数据

   ```lang-java
   package com.sequoiadb.util;
   
   import org.bson.BSONObject;
   import org.bson.util.JSON;
   
   import com.sequoiadb.base.CollectionSpace;
   import com.sequoiadb.base.DBCollection;
   import com.sequoiadb.base.Sequoiadb;
   import com.sequoiadb.exception.BaseException;
   
   public class Sample {
   	public static void main(String[] args) {
           String connString = "192.168.81.134:11810";
           try {
               Sequoiadb sdb = new Sequoiadb(connString, "sdbadmin", "sdbadmin");
               // 获取 school 集合空间对象
               CollectionSpace cs = sdb.getCollectionSpace("school");
               // 获取 student 集合对象
               DBCollection cl = cs.getCollection("student");
               // 删除 id 为 1 记录
               BSONObject queryCondition = (BSONObject) JSON.parse("{id:1}");
               cl.delete(queryCondition);
           } catch (BaseException e) {
               System.out.println("Sequoiadb driver error, error description:" + e.getErrorType());
           }
       }
   }
   ```

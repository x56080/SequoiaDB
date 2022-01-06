
Java 驱动的连接池给提供用户一个快速获取连接实例的途径。

##连接池用法##

使用类 SequoiadbDatasource 的 getConnection 方法从连接池中获取一个连接，使用 releaseConnection 方法把取出的连接放回连接池。当连接池使用的连接数到达连接上限时，下一个请求连接的操作将会等待一段时间（默认超时时间为 5s），若在规定的时间内无空闲的连接可用，将抛出异常。

类 ConfigOptions 可以设置建立连接的各项参数，类 DatasourceOptions 可以设置连接池的各种参数，详情可查看相关 [Java API][api] 介绍。

> **Note:**
>
> 所有使用连接池的客户机器都需要在本地配置服务端协调节点的主机名/IP地址映射关系。

##示例##

```lang-java
package com.sequoiadb.samples;

import java.util.ArrayList;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.SequoiadbDatasource;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ConfigOptions;

public class Datasource {
    public static void main(String[] args) throws InterruptedException {
        ArrayList<String> addrs = new ArrayList<String>();
        String user = "";
        String password = "";
        ConfigOptions nwOpt = new ConfigOptions();
        DatasourceOptions dsOpt = new DatasourceOptions();
        SequoiadbDatasource ds = null;
        // 提供coord节点地址	
        addrs.add("192.168.20.165:11810");
        addrs.add("192.168.20.166:11810");
        addrs.add("ubuntu1504:11810");

        // 设置网络参数
        nwOpt.setConnectTimeout(500);                      // 建连超时时间为 500ms
        nwOpt.setMaxAutoConnectRetryTime(0);               // 建连失败后重试时间为 0ms

        // 设置连接池参数
        dsOpt.setMaxCount(500);                            // 连接池最多能提供 500 个连接
        dsOpt.setDeltaIncCount(20);                        // 每次增加 20 个连接
        dsOpt.setMaxIdleCount(20);                         // 连接池空闲时，保留 20 个连接 

        dsOpt.setKeepAliveTimeout(0);                      // 池中空闲连接存活时间，单位:毫秒 
                                                           // 0 表示不关心连接隔多长时间没有收发消息
 
        dsOpt.setCheckInterval(60 * 1000);                 // 每隔 60s 将连接池中多于 MaxIdleCount 限定的
                                                           // 空闲连接关闭，并将存活时间过长（连接已停止收
                                                           // 发超过 keepAliveTimeout 时间）的连接关闭
                                                           
        dsOpt.setSyncCoordInterval(0);                     // 向 catalog 同步 coord 地址的周期，单位:毫秒 
                                                           // 0 表示不同步 

        dsOpt.setValidateConnection(false);                // 连接出池时，是否检测连接的可用性，默认不检测
        dsOpt.setConnectStrategy(ConnectStrategy.BALANCE); // 默认使用 coord 地址负载均衡的策略获取连接
        
        // 建立连接池
        ds = new SequoiadbDatasource(addrs, user, password, nwOpt, dsOpt);
        
        // 使用连接池运行任务
        runTask(ds);
        
        // 任务结束后，关闭连接池
        ds.close();
    }
    
    static void runTask(SequoiadbDatasource ds) throws InterruptedException {
        String clFullName = "mycs.mycl";
        // 准备任务
        Thread createCLTask = new Thread(new CreateCLTask(ds, clFullName));
        Thread insertTask = new Thread(new InsertTask(ds, clFullName));
        Thread queryTask = new Thread(new QueryTask(ds, clFullName));
        
        // 创建集合
        createCLTask.start();
        createCLTask.join();
        
        // 往集合插记录
        insertTask.start();
        Thread.sleep(3000);
        
        // 从集合中查记录
        queryTask.start();
        
        // 等待任务结束
        insertTask.join();
        queryTask.join();
    }
}

class CreateCLTask implements Runnable {
    private SequoiadbDatasource ds;
    private String csName;
    private String clName;
    
    public CreateCLTask(SequoiadbDatasource ds, String clFullName) {
        this.ds = ds;
        this.csName = clFullName.split("\\.")[0];
        this.clName = clFullName.split("\\.")[1];
    }
    
    @Override
    public void run() {
        Sequoiadb db = null;
        CollectionSpace cs = null;
        DBCollection cl = null;
        // 从连接池获取连接池
        try {
            db = ds.getConnection();
        } catch (BaseException e) {
            e.printStackTrace();
            System.exit(1);
        } catch (InterruptedException e) {
            e.printStackTrace();
            System.exit(1);
        }
        // 使用连接创建集合
        if (db.isCollectionSpaceExist(csName))
            db.dropCollectionSpace(csName);
        cs = db.createCollectionSpace(csName);
        cl = cs.createCollection(clName);
        // 将连接归还连接池
        ds.releaseConnection(db);
        System.out.println("Suceess to create collection " + csName + "." + clName);
    }
}

class InsertTask implements Runnable {
    private SequoiadbDatasource ds;
    private String csName;
    private String clName;
    
    public InsertTask(SequoiadbDatasource ds, String clFullName) {
        this.ds = ds;
        this.csName = clFullName.split("\\.")[0];
        this.clName = clFullName.split("\\.")[1];
    }
    
    @Override
    public void run() {
        Sequoiadb db = null;
        CollectionSpace cs = null;
        DBCollection cl = null;
        BSONObject record = null;
        // 从连接池获取连接
        try {
            db = ds.getConnection();
        } catch (BaseException e) {
            e.printStackTrace();
            System.exit(1);
        } catch (InterruptedException e) {
            e.printStackTrace();
            System.exit(1);
        }
        
        // 使用连接获取集合对象
        cs = db.getCollectionSpace(csName);
        cl = cs.getCollection(clName);
        // 使用集合对象插入记录
        record = genRecord();
        cl.insert(record);
        // 将连接归还连接池
        ds.releaseConnection(db);
        System.out.println("Suceess to insert record: " + record.toString());
    }
	
    private BSONObject genRecord() {
        BSONObject obj = new BasicBSONObject();
        obj.put("name", "James");
        obj.put("age", 30);
        return obj;
    }
}

class QueryTask implements Runnable {
    private SequoiadbDatasource ds;
    private String csName;
    private String clName;
    
    public QueryTask(SequoiadbDatasource ds, String clFullName) {
        this.ds = ds;
        this.csName = clFullName.split("\\.")[0];
        this.clName = clFullName.split("\\.")[1];
    }
    
    @Override
    public void run() {
        Sequoiadb db = null;
        CollectionSpace cs = null;
        DBCollection cl = null;
        DBCursor cursor = null;
        // 从连接池获取连接
        try {
            db = ds.getConnection();
        } catch (BaseException e) {
            e.printStackTrace();
            System.exit(1);
        } catch (InterruptedException e) {
            e.printStackTrace();
            System.exit(1);
        }
        // 使用连接获取集合对象
        cs = db.getCollectionSpace(csName);
        cl = cs.getCollection(clName);
        // 使用集合对象查询
        cursor = cl.query();
        try {
            while(cursor.hasNext()) {
            System.out.println("The inserted record is: " + cursor.getNext());
            }
        } finally {
           cursor.close();
        }
        // 将连接对象归还连接池
        ds.releaseConnection(db);
    }
}
```


[^_^]:
     本文使用的所有引用和链接
[api]:api/java/html/index.html

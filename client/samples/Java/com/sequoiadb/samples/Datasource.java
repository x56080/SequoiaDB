/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = Datasource.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.samples;

import java.util.ArrayList;
import com.sequoiadb.base.*;
import com.sequoiadb.datasource.SequoiadbDatasource;
import org.bson.BasicBSONObject;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.exception.BaseException;

class TaskBase {
    protected SequoiadbDatasource ds;
    protected String csName;
    protected String clName;

    public TaskBase(SequoiadbDatasource ds, String tableName) {
        this.ds = ds;
        this.csName = tableName.split("\\.")[0];
        this.clName = tableName.split("\\.")[1];
    }
}

class InsertTask extends TaskBase implements Runnable {
    public InsertTask(SequoiadbDatasource ds, String tableName) {
        super(ds, tableName);
    }

    @Override
    public void run() {
        Sequoiadb db = null;
        // 从连接池获取连接
        try {
            db = ds.getConnection();
            // 获取集合对象
            CollectionSpace cs = db.getCollectionSpace(csName);
            DBCollection cl = cs.getCollection(clName);
            // 插入记录
            cl.insertRecord(new BasicBSONObject().append("name", "James").append("age", 37));
        } catch (BaseException | InterruptedException e) {
            e.printStackTrace();
            System.exit(1);
        } finally {
            // 将连接归还连接池
            if (db != null) {
                ds.releaseConnection(db);
            }
        }
        System.out.println("Success to insert records");
    }
}

class QueryTask extends TaskBase implements Runnable {
    public QueryTask(SequoiadbDatasource ds, String tableName) {
        super(ds, tableName);
    }

    @Override
    public void run() {
        Sequoiadb db = null;
        // 从连接池获取连接
        try {
            db = ds.getConnection();
            // 使用连接获取集合对象
            CollectionSpace cs = db.getCollectionSpace(csName);
            DBCollection cl = cs.getCollection(clName);
            // 使用集合对象查询
            DBCursor cursor = cl.query();
            try {
                while(cursor.hasNext()) {
                    System.out.println("The inserted record is: " + cursor.getNext());
                }
            } finally {
                cursor.close();
            }
        } catch (BaseException | InterruptedException e) {
            e.printStackTrace();
            System.exit(1);
        } finally {
            // 将连接对象归还连接池
            if (db != null) {
                ds.releaseConnection(db);
            }
        }
    }
}

public class Datasource {
    static void prepareTable(SequoiadbDatasource ds, String tableName) {
        Sequoiadb db = null;
        String csName = tableName.split("\\.")[0];
        String clName = tableName.split("\\.")[1];

        try {
            // 从连接池获取连接池
            db = ds.getConnection();
            // 删除原有的集合空间
            if (db.isCollectionSpaceExist(csName)) {
                db.dropCollectionSpace(csName);
            }
            // 创建新的集合
            db.createCollectionSpace(csName).createCollection(clName);
        } catch (BaseException | InterruptedException e) {
            e.printStackTrace();
            System.exit(1);
        } finally {
            // 将连接归还连接池
            if (db != null) {
                ds.releaseConnection(db);
            }
        }
        System.out.println("Success to create collection " + csName + "." + clName);
    }

    static void runTask(SequoiadbDatasource ds, String tableName) throws InterruptedException {
        // 创建表
        prepareTable(ds, tableName);

        // 准备插入、查询任务
        Thread insertTask = new Thread(new InsertTask(ds, tableName));
        Thread queryTask = new Thread(new QueryTask(ds, tableName));

        // 运行插入、查询任务
        insertTask.start();
        Thread.sleep(3000);
        queryTask.start();

        // 等待任务结束
        insertTask.join();
        queryTask.join();
    }

    public static void main(String[] args) throws InterruptedException {
        SequoiadbDatasource ds = null;
        String userName = "admin";
        String password = "admin";
        String tableName = "sample.employee";

        // 提供 SequoiaDB 集群协调节点地址
        ArrayList< String > addrList = new ArrayList< String >();
        addrList.add( "sdbserver1:11810" );
        addrList.add( "sdbserver2:11810" );
        addrList.add( "sdbserver3:11810" );

        // 连接池参数配置
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMaxCount( 500 );
        dsOpt.setMaxIdleCount( 50 );
        dsOpt.setMinIdleCount( 20 );

        // 连接参数配置
        ConfigOptions nwOpt = new ConfigOptions();
        nwOpt.setConnectTimeout( 200 );
        nwOpt.setMaxAutoConnectRetryTime( 0 );

        // 创建连接池对象
        ds = new SequoiadbDatasource( addrList, userName, password, nwOpt, dsOpt );
        try {
            // 使用连接池
            runTask(ds, tableName);
        } finally {
            // 关闭连接池
            ds.close();
        }
    }
}
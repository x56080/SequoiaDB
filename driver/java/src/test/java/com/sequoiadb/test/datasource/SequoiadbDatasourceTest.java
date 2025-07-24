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

   Source File Name = SequoiadbDatasourceTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.datasource;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.Helper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicLong;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;

public class SequoiadbDatasourceTest {
    private SequoiadbDatasource ds;
    private static List<String> coords = new ArrayList<String>();

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        coords.add(Constants.COOR_NODE_CONN);
        coords.add("192.168.20.166:50000");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
        try {
            ds = new SequoiadbDatasource(coords, "", "", null, (DatasourceOptions) null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @After
    public void tearDown() throws Exception {
        if (ds != null) {
            ds.close();
        }
    }

    @Test
    public void getLastReleaseConnection() throws InterruptedException {
        Sequoiadb sdb1 = ds.getConnection();
        ds.releaseConnection(sdb1);
        Sequoiadb sdb2 = ds.getConnection();
        ds.releaseConnection(sdb2);
        System.out.println("hashcode of sdb1 is: " + sdb1.hashCode());
        System.out.println("hashcode of sdb2 is: " + sdb2.hashCode());
        Assert.assertEquals(sdb1, sdb2);
    }

    @Test
    public void setSessionAttrInDatasource() {
        int threadCount = 50;
        int maxCount = 50;
        DatasourceOptions options = new DatasourceOptions();
        options.setMaxCount(maxCount);
        options.setPreferedInstance(Arrays.asList("M", "m", "1", "2", "012"));
        options.setPreferedInstanceMode("ordered");
        options.setSessionTimeout(100);
        SequoiadbDatasource sds = new SequoiadbDatasource(coords, "", "", null, options);
        Sequoiadb[] dbs = new Sequoiadb[threadCount];
        for(int i = 0; i < threadCount; i++) {
            try {
                dbs[i] = sds.getConnection();
            } catch (Exception e) {
                System.out.println("i is: " + i);
                e.printStackTrace();
                Assert.assertFalse(true);
            }
        }
        for(int i = 0; i < threadCount; i++) {
            sds.releaseConnection(dbs[i]);
        }
        sds.close();
    }

    // jira-2136
    @Test
    @Ignore
    public void jira2136_transactionRollback() throws InterruptedException {
        DatasourceOptions dsOpts = new DatasourceOptions();
        dsOpts.setMaxCount(1);
        dsOpts.setDeltaIncCount(1);
        dsOpts.setMaxIdleCount(1);
        ds.updateDatasourceOptions(dsOpts);
        Sequoiadb db = ds.getConnection();
        String csName = "jira2136";
        String clName = "jira2136";
        CollectionSpace cs = Helper.getOrCreateCollectionSpace(db, csName, null);
        DBCollection cl = Helper.getOrCreateCollection(cs, clName, new BasicBSONObject("ReplSize", 0));
        db.beginTransaction();
        cl.insert(new BasicBSONObject("a", 1));
        ds.releaseConnection(db);
        db = ds.getConnection();
        cl = db.getCollectionSpace(csName).getCollection(clName);
        long recordCount = cl.getCount();
        Assert.assertEquals(0, recordCount);
        db.dropCollectionSpace(csName);
        ds.releaseConnection(db);
    }

    /*
     * connect one
     * */
    @Test
    public void testConnectOne() throws BaseException, InterruptedException {
        Sequoiadb sdb = ds.getConnection();
        CollectionSpace cs;
        // cs
        if (sdb.isCollectionSpaceExist("ds")) {
            sdb.dropCollectionSpace("ds");
            cs = sdb.createCollectionSpace("ds");
        } else {
            cs = sdb.createCollectionSpace("ds");
        }
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        DBCollection cl = cs.createCollection("ds", conf);

        BSONObject obj = new BasicBSONObject();
        Integer i1 = 10;
        obj.put("Id", i1);
        obj.put("Age", 30);

        cl.insert(obj);

        DBCursor cursor = cl.query();
        int i = 0;
        while (cursor.hasNext()) {
            BSONObject record = cursor.getNext();
            System.out.print(record);
            i++;
        }
        assertEquals(1, i);

        sdb.dropCollectionSpace("ds");
        ds.releaseConnection(sdb);
    }

    static AtomicLong l = new AtomicLong(0);

    class ReleaseResourceTestTask implements Runnable {
        Random random = new Random();
        SequoiadbDatasource _ds;

        ReleaseResourceTestTask(SequoiadbDatasource myds) {
            _ds = myds;
        }

        @Override
        public void run() {
            while (true) {
                Sequoiadb sdb = null;
                try {
//                    sdb = _ds.getConnection(0);
                    sdb = _ds.getConnection();
                    System.out.println("thread:" + Thread.currentThread().getName() + ", ok - " + l.getAndAdd(1));
                    try {
                        Thread.sleep(random.nextInt(10 * 1000));
                    } catch (InterruptedException e) {
                    }
                    DBCursor cursor = sdb.listCollections();
                    while(cursor.hasNext()) {
                        cursor.getNext();
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    if (!(e instanceof BaseException)) {
                        System.out.println(String.format("thread[%d] exit!", Thread.currentThread().getId()));
                        System.exit(-1);
                    }
                }
                if (_ds != null) {
                    int abnormalAddrCount = _ds.getAbnormalAddrNum();
                    int normalAddrCount = _ds.getNormalAddrNum();
                    System.out.println("normal address count is: " + normalAddrCount +
                            ", abnormal address count is: " + abnormalAddrCount);
                }
                if (sdb != null) {
                    try {
                        _ds.releaseConnection(sdb);
                    }catch (BaseException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    @Test
    @Ignore
    public void jira_2797_releaseResourceTest() throws InterruptedException {
        List<String> list = new ArrayList<String>();
        list.add("192.168.20.166:11810");
        list.add("192.168.20.166:50000");
        list.add("192.168.20.166:40000");
        list.add("192.168.20.166:30000");
        DatasourceOptions options = new DatasourceOptions();
        options.setConnectStrategy(ConnectStrategy.BALANCE);
//        options.setConnectStrategy(ConnectStrategy.SERIAL);
//        options.setConnectStrategy(ConnectStrategy.RANDOM);
        options.setMaxCount(100);
        options.setDeltaIncCount(10);
        options.setCheckInterval(30 * 1000);
        options.setKeepAliveTimeout(60 * 1000);
        options.setMaxIdleCount(10);
        options.setValidateConnection(true);
        ConfigOptions configOptions = new ConfigOptions();
        configOptions.setSocketTimeout(10);
        SequoiadbDatasource myds = new SequoiadbDatasource(list, "", "", null, options);
        myds.disableDatasource();

        int threadCount = 120;
        Thread[] threads = new Thread[threadCount];
        for (int i = 0; i < threadCount; i++) {
            threads[i] = new Thread(new ReleaseResourceTestTask(myds), "" + i);
        }
        for (int i = 0; i < threadCount; i++) {
            threads[i].start();
        }
        for (int i = 0; i < threadCount; i++) {
            threads[i].join();
        }
//        while(true) {
//            System.out.println("###############################################################enable");
//            myds.enableDatasource();
//            try {
//                Thread.sleep(20 * 1000);
//            } catch (InterruptedException e) {
//            }
//            System.out.println("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$disable");
//            myds.disableDatasource();
//            try {
//                Thread.sleep(20 * 1000);
//            } catch (InterruptedException e) {
//            }
//        }
        try {
            Thread.sleep(300 * 1000);
        } catch (InterruptedException e) {
        }
    }

    @Test
    public void jira_2863_missing_a_connection() {
        ArrayList<Sequoiadb> dbs = new ArrayList<Sequoiadb>();
        DatasourceOptions options = null;
        int poolSize = 0;
        try {
            options = (DatasourceOptions) ds.getDatasourceOptions();
            poolSize = options.getMaxCount();
            //申请到池满
            for (int i = 0; i < poolSize; ++i) {
                Sequoiadb db = ds.getConnection();
                Assert.assertEquals(db.isValid(), true);
                dbs.add(db);
            }
//            System.out.println(String.format("Total: %d, create directly: %d", poolSize, ds.aInt.get()));
            for(Sequoiadb db : dbs) {
                ds.releaseConnection(db);
            }
        } catch (InterruptedException e) {
            System.out.println("current get connection number " + dbs.size());
            e.printStackTrace();
            assertFalse(e.getMessage(), true);
        } catch (BaseException e) {
            System.out.println("current get connection number " + dbs.size());
            e.printStackTrace();
            throw e;
        }
    }

    @Test
    @Ignore
    public void  getConnectionsPerformanceTesting() throws InterruptedException {
        String addr = Constants.COOR_NODE_CONN;
        int connNum = 500;

        // case 1: create connection directly
        Sequoiadb[] dbs = new Sequoiadb[connNum];
        long beginTime = System.currentTimeMillis();
        for (int i = 0; i < connNum; i++) {
            dbs[i] = new Sequoiadb(addr, "", "");
        }
        long endTime = System.currentTimeMillis();
        System.out.println(String.format("create connections directly takes: %dms", endTime - beginTime));

        // case 2: get connections from data source
        List<String> coords = new ArrayList<String>();
        coords.add(Constants.COOR_NODE_CONN);
        DatasourceOptions options = new DatasourceOptions();
        SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, options);
        beginTime = System.currentTimeMillis();
        for (int i = 0; i < connNum; i++) {
            dbs[i] = datasource.getConnection();
        }
        endTime = System.currentTimeMillis();
        System.out.println(String.format("get connetions from data source takes: %dms", endTime - beginTime));
        // release connections
        for (int i = 0; i < connNum; i++) {
            datasource.releaseConnection(dbs[i]);
        }
        datasource.close();

        // case 3:
        options = new DatasourceOptions();
        options.setMaxIdleCount(options.getMaxCount());
        datasource = new SequoiadbDatasource(coords, "", "", null, options);
        for (int i = 0; i < connNum; i++) {
            dbs[i] = datasource.getConnection();
        }
        for (int i = 0; i < connNum; i++) {
            datasource.releaseConnection(dbs[i]);
        }
        Assert.assertEquals(connNum, datasource.getIdleConnNum());
        beginTime = System.currentTimeMillis();
        for (int i = 0; i < connNum; i++) {
            dbs[i] = datasource.getConnection();
        }
        endTime = System.currentTimeMillis();
        System.out.println(String.format("get connetions from data source cache takes: %dms", endTime - beginTime));
        // release connections
        for (int i = 0; i < connNum; i++) {
            datasource.releaseConnection(dbs[i]);
        }
        datasource.close();

    }

    /*
    * check:
    * 1. datasource keep the idle connection num or not
    * */
    @Test
    @Ignore
    public void jira_6721_features1() {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval(10 * 1000);
        SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        Sequoiadb sdb = null;
        int sleepTime = 240 * 1000;
        try {
            try {
                sdb = datasource.getConnection();
            } finally {
                datasource.releaseConnection(sdb);
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
            Assert.fail();
        }
        while (sleepTime > 0) {
            int interval = 5 * 1000;
            try {
                Thread.sleep(interval);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                    ", dsOpt.getMinIdleCount(): " + dsOpt.getMinIdleCount(),datasource.getIdleConnNum() >= dsOpt.getMinIdleCount());
            sleepTime -= interval;
        }
        Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                ", dsOpt.getMaxIdleCount(): " + dsOpt.getMaxIdleCount(),datasource.getIdleConnNum() <= dsOpt.getMaxIdleCount());
    }

    /*
     * check:
     * 1. cacheLimit works or not
     * 2. background thread create connection or not(minIdleCount == maxIdleCount)
     * */
    @Test
    @Ignore
    public void jira_6721_features2() {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval(10 * 1000);
        dsOpt.setCacheLimit(10);
        SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        List<Sequoiadb> sdbList = new ArrayList<Sequoiadb>();
        int sleepTime = 240 * 1000;
        try {
            int i = 0;
            try {
                while(i < dsOpt.getMaxCount()) {
                    sdbList.add(datasource.getConnection());
                    i++;
                }
            } finally {
                while (sdbList.size() > 0) {
                    datasource.releaseConnection(sdbList.get(0));
                    sdbList.remove(0);
                }
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum(),
                    datasource.getIdleConnNum() == 0);
        } catch (InterruptedException e) {
            e.printStackTrace();
            Assert.fail();
        }
        while (sleepTime > 0) {
            int interval = 15 * 1000;
            try {
                Thread.sleep(interval);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                    ", dsOpt.getMinIdleCount(): " + dsOpt.getMinIdleCount(),datasource.getIdleConnNum() >= dsOpt.getMinIdleCount());
            sleepTime -= interval;
        }
        Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                ", dsOpt.getMaxIdleCount(): " + dsOpt.getMaxIdleCount(),datasource.getIdleConnNum() <= dsOpt.getMaxIdleCount());
    }

    /*
     * check:
     * 1. cacheLimit works or not
     * 2. background thread create connection or not(minIdleCount != maxIdleCount)
     * */
    @Test
    @Ignore
    public void jira_6721_features3() {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval(10 * 1000);
        dsOpt.setCacheLimit(10);
        dsOpt.setMaxIdleCount(20);
        dsOpt.setMinIdleCount(10);
        SequoiadbDatasource datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        List<Sequoiadb> sdbList = new ArrayList<Sequoiadb>();
        int sleepTime = 240 * 1000;
        try {
            int i = 0;
            try {
                while(i < dsOpt.getMaxCount()) {
                    sdbList.add(datasource.getConnection());
                    i++;
                }
            } finally {
                while (sdbList.size() > 0) {
                    datasource.releaseConnection(sdbList.get(0));
                    sdbList.remove(0);
                }
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum(),
                    datasource.getIdleConnNum() == 0);
        } catch (InterruptedException e) {
            e.printStackTrace();
            Assert.fail();
        }
        while (sleepTime > 0) {
            int interval = 15 * 1000;
            try {
                Thread.sleep(interval);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                    ", dsOpt.getMinIdleCount(): " + dsOpt.getMinIdleCount(),datasource.getIdleConnNum() >= dsOpt.getMinIdleCount());
            sleepTime -= interval;
        }
        Assert.assertTrue("datasource.getIdleConnNum(): " + datasource.getIdleConnNum() +
                ", dsOpt.getMaxIdleCount(): " + dsOpt.getMaxIdleCount(),datasource.getIdleConnNum() <= dsOpt.getMaxIdleCount());
    }

    /*
    * test:
    * 1. minIdleCount
    * */
    @Test
    public void jira_6721_features4() {
        SequoiadbDatasource datasource;
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setCheckInterval(10 * 1000);
        dsOpt.setCacheLimit(10);

        // case 1: less than 0
        try {
            dsOpt.setMinIdleCount(11);
            datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // case 2: max than maxIdleCount
        try {
            dsOpt.setMinIdleCount(11);
            datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }


        // case 3: max than maxCount
        try {
            dsOpt.setMinIdleCount(501);
            dsOpt.setMaxIdleCount(501);
            datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // case 4: equal to 0
        dsOpt.setMinIdleCount(0);
        dsOpt.setMaxIdleCount(10);
        datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        datasource.close();

        // case 5: equal to maxIdleCount
        dsOpt.setMinIdleCount(100);
        dsOpt.setMaxIdleCount(100);
        datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        datasource.close();

        // case 6: equal to maxCount
        dsOpt.setMinIdleCount(500);
        dsOpt.setMaxIdleCount(500);
        datasource = new SequoiadbDatasource(coords, "", "", null, dsOpt);
        datasource.close();

    }

}

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

   Source File Name = ConnectionBrokenTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.datasource;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.SequoiadbDatasource;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Created by tanzhaobo on 2018/1/24.
 */
public class ConnectionBrokenTest {

    private SequoiadbDatasource ds;
    private static List<String> coords = new ArrayList<String>();
    private static String hostName = "192.168.20.165";
    private static int coordPort = 11810;
    private static int dataPort = 20000;
    private static String csName = "testfoo";
    private static String clName = "testbar";
    private static String groupName = "group2";
    private Sequoiadb db;

    @BeforeClass
    public static void beforeClass() {
        coords.add(hostName + ":" + coordPort);
    }

    @AfterClass
    public static void afterClass() {
    }

    @Before
    public void setUp() {
        // prepare connection
        db = new Sequoiadb(hostName, coordPort, "", "");
        if (db.isCollectionSpaceExist(csName)) {
            db.dropCollectionSpace(csName);
        }
        CollectionSpace cs = db.createCollectionSpace(csName);
        cs.createCollection(clName, new BasicBSONObject("Group", groupName));
        // prepare datasource
        DatasourceOptions datasourceOptions = new DatasourceOptions();
        datasourceOptions.setMaxCount(2000);
        datasourceOptions.setDeltaIncCount(500);
//        datasourceOptions.setMaxIdleCount(3);
        datasourceOptions.setKeepAliveTimeout(30000);
        datasourceOptions.setCheckInterval(10000);
        try {
            ds = new SequoiadbDatasource(coords, "", "", null, datasourceOptions);
            ds.releaseConnection(ds.getConnection());
            Thread.sleep(20000);
            System.out.println(String.format("Datasource has prepare %d connections", ds.getIdleConnNum()));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @After
    public void tearDown() {
        // close connection
        if (db != null) {
            try {
                db.dropCollectionSpace(csName);
            } catch (Exception e) {
                // ignore
            }
            db.disconnect();
        }
        // close datasource
        if (ds != null) {
            ds.close();
        }
    }

    @Test
    @Ignore
    public void connectBrokenTest() throws InterruptedException {
        Sequoiadb sdb = new Sequoiadb(hostName, dataPort, "", "");
        int initialValue = getSessionCount(sdb);
        sdb.disconnect();
        int count = 501;
        ExecutorService executorService = Executors.newCachedThreadPool();
        executorService.execute(new Worker(ds, csName, clName, count, 0));
        executorService.execute(new Snapshot(count, initialValue));
        executorService.shutdown();
        Thread.sleep(1200 * 1000);
    }

    class Worker implements Runnable {
        SequoiadbDatasource sds;
        String csName;
        String clName;
        int count;
        int sleepSec;

        Worker(SequoiadbDatasource sds, String csName, String clName, int count, int sleepSec) {
            this.sds = sds;
            this.csName = csName;
            this.clName = clName;
            this.count = count;
            this.sleepSec =  sleepSec;
        }

        @Override
        public void run() {

            while (true) {
                Sequoiadb[] dbs = new Sequoiadb[count];
                for(int i = 0; i < count; i++) {
                    Sequoiadb db = null;
                    try {
                        db = sds.getConnection();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    dbs[i] = db;
                    db.getCollectionSpace(csName).getCollection(clName).insert(new BasicBSONObject("a", i));
                }
                for(int i = 0; i < count; i++) {
                    sds.releaseConnection(dbs[i]);
                }
                try {
                    Thread.sleep(sleepSec * 1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    class Snapshot implements Runnable {
        private int count;
        private int initialValue;

        public Snapshot(int count, int initialValue) {
            this.count = count;
            this.initialValue = initialValue;
        }

        @Override
        public void run() {
            Sequoiadb sdb = new Sequoiadb(hostName, dataPort, "", "");
            int retryTime = 0;
            while(true) {
                System.out.println("times: " + ++retryTime);
                int counter = getSessionCount(sdb);
                if (counter < count) {
                    System.out.println(String.format("expect greater than: %d, actual: %d", count, counter));
                } else if (counter > count + initialValue) {
                    System.out.println(String.format("initial valueis: %d, expect less than: %d, actual: %d",
                            initialValue, count + initialValue, counter));
                }
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    private int getSessionCount(Sequoiadb db) {
        DBCursor cursor = db.getSnapshot(Sequoiadb.SDB_SNAP_SESSIONS, "", "", "");
        int counter = 0;
        while (cursor.hasNext()) {
            ++counter;
            cursor.getNext();
        }
        return counter;
    }


}

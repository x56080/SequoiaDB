package com.sequoiadb.test.datasource;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicLong;

import static org.junit.Assert.assertEquals;

public class SequoiadbDatasourceTest {
    private SequoiadbDatasource ds;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
        List<String> coords = new ArrayList<String>();
        coords.add(Constants.COOR_NODE_CONN);

        try {
            ds = new SequoiadbDatasource(coords, "", "", null, (DatasourceOptions) null);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @After
    public void tearDown() throws Exception {
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
        obj.put("Id", 10);
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

        ReleaseResourceTestTask(SequoiadbDatasource ds) {
            _ds = ds;
        }

        @Override
        public void run(){
            while(true) {
                Sequoiadb sdb = null;
                try {
                    sdb = _ds.getConnection();
                    System.out.println( "thread:" + Thread.currentThread().getName() + ", ok - " + l.getAndAdd(1));
                    try {
                        Thread.sleep(random.nextInt(10 * 1000));
                    } catch (InterruptedException e) {
                    }
                } catch (Exception e) {
//                    e.printStackTrace();
                    if (e instanceof BaseException) {
                        System.out.println("thread:" + Thread.currentThread().getName() + ", " +
                                l.getAndAdd(1) + " - error number: " + ((BaseException)e).getErrorCode() +
                                ", error msg: " + ((BaseException)e).getMessage());
                    } else {
//                        e.printStackTrace();
                        System.out.println("thread:" + Thread.currentThread().getName() +
                                ","  + l.getAndAdd(1) + " - error.");
                    }
                }
                if (_ds != null) {
                    int abnormalAddrCount = _ds.getAbnormalAddrNum();
                    int normalAddrCount = _ds.getNormalAddrNum();
                    System.out.println("normal address count is: " + normalAddrCount +
                            ", abnormal address count is: " + abnormalAddrCount);
                }
                if (sdb != null) {
                    _ds.releaseConnection(sdb);
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
        DatasourceOptions options = new DatasourceOptions();
//        options.setConnectStrategy(ConnectStrategy.SERIAL);
        options.setValidateConnection(true);
        SequoiadbDatasource ds = new SequoiadbDatasource(list, "", "", null, options);

        int threadCount = 30;
        Thread[] threads = new Thread[threadCount];
        for (int i = 0; i < threadCount; i++) {
            threads[i] = new Thread(new ReleaseResourceTestTask(ds), "" + i);
        }
        for (int i = 0; i < threadCount; i++) {
            threads[i].start();
        }
        for (int i = 0; i < threadCount; i++) {
            threads[i].join();
        }
        try {
            Thread.sleep(300 * 1000);
        } catch (InterruptedException e) {
        }
    }
}

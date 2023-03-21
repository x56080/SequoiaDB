package com.sequoiadb.test.datasource;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.ConnectStrategy;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.test.common.Constants;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;

public class ConcurrencyTest {
    private final static int PORT1 = 51000;
    private final static int PORT2 = 52000;
    private final static int PORT3 = 53000;
    private final static int PORT4 = 54000;
    private final static int PORT5 = 55000;
    private static Sequoiadb db;
    private static ReplicaGroup coordRG;
    private static Node node1;
    private static Node node2;
    private static Node node3;
    private static Node node4;
    private static Node node5;
    private SequoiadbDatasource ds;
    private ConfigOptions netConfig;
    private DatasourceOptions options;

    @BeforeClass
    public static void prepareBeforeClass() {
        db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        coordRG = db.getReplicaGroup("SYSCoord");

        String dbPath = Constants.DB_PATH + "/coord/";
        node1 = coordRG.createNode(Constants.HOST, PORT1, dbPath + PORT1);
        node2 = coordRG.createNode(Constants.HOST, PORT2, dbPath + PORT2);
        node3 = coordRG.createNode(Constants.HOST, PORT3, dbPath + PORT3);
        node4 = coordRG.createNode(Constants.HOST, PORT4, dbPath + PORT4);
        node5 = coordRG.createNode(Constants.HOST, PORT5, dbPath + PORT5);
    }

    @AfterClass
    public static void cleanAfterClass() {
        try {
            coordRG.removeNode(Constants.HOST, PORT1, null);
            coordRG.removeNode(Constants.HOST, PORT2, null);
            coordRG.removeNode(Constants.HOST, PORT3, null);
            coordRG.removeNode(Constants.HOST, PORT4, null);
            coordRG.removeNode(Constants.HOST, PORT5, null);
        } finally {
            db.close();
        }
    }

    @Before
    public void setUp() {
        netConfig = new ConfigOptions();
        netConfig.setConnectTimeout(100);
        netConfig.setMaxAutoConnectRetryTime(200);
        options = new DatasourceOptions();
        options.setConnectStrategy(ConnectStrategy.SERIAL);

        coordRG.start();
        node3.stop();
    }

    @After
    public void tearDown() {
        if (ds != null) {
            ds.close();
        }
    }

    @Test
    public void test() throws Exception {
        List<String> addressList = new ArrayList<>();
        addressList.add(node1.getNodeName());
        addressList.add(node2.getNodeName());
        addressList.add(node3.getNodeName());
        addressList.add(node4.getNodeName());
        addressList.add(node5.getNodeName());

        options.setSyncCoordInterval(100);
        options.setMaxIdleCount(3);
        options.setMinIdleCount(3);
        options.setCheckInterval(10);
        ds = SequoiadbDatasource.builder()
                .serverAddress(addressList)
                .configOptions(netConfig)
                .datasourceOptions(options)
                .build();

        testDS(ds, 50,1000, "Enable test");

        options.setMaxCount(100);
        ds.updateDatasourceOptions(options);
        ds.disableDatasource();
        testDS(ds, 10,40, "Disable test");
    }

    public void testDS(SequoiadbDatasource ds, int threadNum, int cycleTime, String msg) throws Exception {
        List<String> addressList = new ArrayList<>();
        addressList.add(node2.getNodeName());
        addressList.add(node3.getNodeName());
        addressList.add(node4.getNodeName());
        addressList.add(node5.getNodeName());

        int num = ds.getDatasourceOptions().getMaxCount() / threadNum;
        List<Worker> workerList = new ArrayList<>();
        for (int i = 0; i < threadNum; i++) {
            workerList.add(new Worker(ds, addressList.get(i % 4), num, cycleTime));
        }

        long startTime = System.currentTimeMillis();
        for (Worker worker: workerList) {
            worker.start();
        }
        for (Worker worker: workerList) {
            worker.join();
        }
        long endTime = System.currentTimeMillis();
        System.out.println(msg + ", time:" + (endTime - startTime) + "ms");
    }

    static class Worker extends Thread {
        private final SequoiadbDatasource ds;
        private final String address;
        private final int num;
        private final int cycleTime;

        Worker(SequoiadbDatasource ds, String address, int num, int cycleTime) {
            this.ds = ds;
            this.address = address;
            this.num = num;
            this.cycleTime = cycleTime;
        }

        @Override
        public void run() {
            List<Sequoiadb> connList = new ArrayList<>();
            for (int i = 0; i < cycleTime; i++) {
                if (i % 10 == 0) {
                    ds.addCoord(address);
                    ds.removeCoord(address);
                } else {
                    for (int j = 0; j < num; j++) {
                        try {
                            connList.add(ds.getConnection(100));
                        } catch (InterruptedException e) {
                            throw new RuntimeException(e);
                        }
                    }
                    for (Sequoiadb db : connList) {
                        ds.releaseConnection(db);
                    }
                    connList.clear();
                }
            }
        }
    }
}

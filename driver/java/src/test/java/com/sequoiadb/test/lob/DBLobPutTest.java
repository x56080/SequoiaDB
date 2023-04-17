package com.sequoiadb.test.lob;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.junit.*;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class DBLobPutTest {

    private static final String CS_NAME = Constants.TEST_CS_NAME_1;
    private static final String CL_NAME = Constants.TEST_CL_NAME_1;
    private static Sequoiadb db;
    private static DBCollection cl;
    private static byte[] emptyData;
    private static byte[] normalData;
    private static byte[] bigData;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");

        emptyData = new byte[0];
        normalData = new byte[100];
        bigData = new byte[2097152 + 1]; // 2MB + 1byte > 2MB

        Arrays.fill(normalData, (byte) 'a');
        Arrays.fill(bigData, (byte) 'a');
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        db.close();
    }

    @Before
    public void setUp() throws Exception {
        if (db.isCollectionSpaceExist(CS_NAME)) {
            db.dropCollectionSpace(CS_NAME);
        }

        BSONObject conf = new BasicBSONObject("Group", Constants.GROUPNAME);
        cl = db.createCollectionSpace(CS_NAME).createCollection(CL_NAME, conf);
    }

    @After
    public void tearDown() throws Exception {
        db.dropCollectionSpace(CS_NAME);
    }

    @Test
    public void normalTest() {
        ObjectId oid;

        // case 1: data is null, oid is null
        try {
            putAndCheck(null, null);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 2: empty data
        putAndCheck( null, emptyData);

        // case 3: normal data, oid is null
        putAndCheck(null, normalData);

        // case 4: normal data and oid
        oid = cl.createLobID();
        putAndCheck(oid, normalData);

        // case 5: oid repeat
        try {
            putAndCheck(oid, normalData);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_FE.getErrorCode(), e.getErrorCode());
        }

        // case 6: big lob
        putAndCheck(null, bigData);
    }

    private void putAndCheck(ObjectId oid, byte[] data) {
        ObjectId retOid = cl.putLob(data, oid);
        if (oid != null) {
            Assert.assertEquals(retOid, oid);
        }
        checkLobData(retOid, data);
    }

    private void checkLobData(ObjectId oid, byte[] data) {
        byte[] actual = new byte[data.length];
        try (DBLob lob = cl.openLob(oid, DBLob.SDB_LOB_READ)) {
            Assert.assertEquals(data.length, lob.getSize());
            lob.read(actual);
            Assert.assertArrayEquals(data, actual);
        }
    }

    @Test
    public void putLobSnapshotTest() {
        byte[] data = "putLobTest".getBytes(StandardCharsets.UTF_8);
        cl.putLob(data);

        ReplicaGroup group = db.getReplicaGroup(Constants.GROUPNAME);
        String nodeName = group.getMaster().getNodeName();

        BSONObject matcher = new BasicBSONObject();
        matcher.put("Name", cl.getFullName());

        BSONObject selector = new BasicBSONObject();
        selector.put("Details.Group.TotalLobs", "");
        selector.put("Details.Group.TotalValidLobSize", "");
        selector.put("Details.Group.TotalLobPut", "");
        selector.put("Details.Group.TotalLobWriteSize", "");
        selector.put("Details.Group.NodeName", "");

        try (DBCursor cursor = db.getSnapshot(Sequoiadb.SDB_SNAP_COLLECTIONS, matcher, selector, null)) {
            BSONObject obj = cursor.getNext();
            BasicBSONList details = (BasicBSONList)obj.get("Details");
            Assert.assertEquals(1, details.size());

            BSONObject groupObj = (BSONObject)details.get(0);
            BasicBSONList infoList = (BasicBSONList)groupObj.get("Group");

            boolean gotInfo = false;
            for (Object o : infoList) {
                BSONObject info = (BSONObject) o;
                String actualNodeName = (String) info.get("NodeName");
                if (!nodeName.equals(actualNodeName)) {
                    continue;
                }
                gotInfo = true;
                long lobNum = (long) info.get("TotalLobs");
                long lobPutNum = (long) info.get("TotalLobPut");
                long lobWriteSize = (long) info.get("TotalLobWriteSize");
                long lobValidSize = (long) info.get("TotalValidLobSize");

                Assert.assertEquals(1, lobNum);
                Assert.assertEquals(1, lobPutNum);
                Assert.assertEquals(data.length, lobWriteSize);
                Assert.assertEquals(data.length, lobValidSize);
            }
            Assert.assertTrue("Error information of SDB_SNAP_COLLECTIONS", gotInfo);
        }
    }

    @Test
    public void putAndReadLobTest() throws InterruptedException {
        putLobConcurrency(LobOpType.READ);
    }

    @Test
    public void putAndWriteLobTest() throws InterruptedException {
        putLobConcurrency(LobOpType.WRITE);
    }

    @Test
    public void putAndTruncateLobTest() throws InterruptedException {
        putLobConcurrency(LobOpType.TRUNCATE);
    }

    @Test
    public void putAndRemoveLobTest() throws InterruptedException {
        putLobConcurrency(LobOpType.REMOVE);
    }

    private void putLobConcurrency(LobOpType type) throws InterruptedException {
        // case 1: normal data
        putLobConcurrencyWithData(normalData, type);

        // case 2: big data
        putLobConcurrencyWithData(bigData, type);
    }

    private void putLobConcurrencyWithData(byte[] data, LobOpType type) throws InterruptedException {
        for (int i = 0; i < 100; i++) {
            ObjectId oid = cl.createLobID();
            LobWorker putLob = new LobWorker(oid, data, LobOpType.PUT);
            LobWorker useLob = new LobWorker(oid, data, type);

            putLob.start();
            useLob.start();

            putLob.join();
            useLob.join();

            try {
                checkLobData(oid, data);
            } catch (BaseException e) {
                if (type == LobOpType.REMOVE) {
                    if (e.getErrorCode() == SDBError.SDB_FNE.getErrorCode()) {
                        continue;
                    }
                }
                throw e;
            }
        }
    }

    static class LobWorker extends Thread {
        private final ObjectId oid;
        private final byte[] data;
        private final LobOpType type;

        LobWorker(ObjectId oid, byte[] data, LobOpType type) {
            this.oid = oid;
            this.data = data;
            this.type = type;
        }

        @Override
        public void run() {
            try (Sequoiadb db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "")){
                DBCollection cl = db.getCollectionSpace(CS_NAME).getCollection(CL_NAME);

                doWork(cl, type);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }

        void doWork(DBCollection cl, LobOpType type) throws InterruptedException {
            switch (type) {
                case PUT:
                    putLob(cl);
                    break;
                case READ:
                    readLob(cl);
                    break;
                case WRITE:
                    writeLob(cl);
                    break;
                case REMOVE:
                    removeLob(cl);
                    break;
                case TRUNCATE:
                    truncateLob(cl);
                    break;
                default:
                    throw new BaseException(SDBError.SDB_INVALIDARG, "Invalid lob operation type");
            }
        }

        void putLob(DBCollection cl) {
            ObjectId retOid = cl.putLob(data, oid);
            Assert.assertEquals(retOid, oid);
        }

        void readLob(DBCollection cl) throws InterruptedException {
            byte[] readData = new byte[data.length];

            // waite for lob create
            Thread.sleep(5);

            try (DBLob lob = cl.openLob(oid, DBLob.SDB_LOB_READ)) {
                lob.read(readData);
                Assert.assertArrayEquals(data, readData);
            } catch (BaseException e) {
                if (e.getErrorCode() != SDBError.SDB_LOB_IS_IN_USE.getErrorCode() &&
                        e.getErrorCode() != SDBError.SDB_FNE.getErrorCode()) {
                    throw e;
                }
            }
        }

        void writeLob(DBCollection cl) throws InterruptedException {
            // waite for lob create
            Thread.sleep(5);

            try (DBLob lob = cl.openLob(oid, DBLob.SDB_LOB_WRITE)) {
                lob.write(data);
            } catch (BaseException e) {
                if (e.getErrorCode() != SDBError.SDB_LOB_IS_IN_USE.getErrorCode() &&
                        e.getErrorCode() != SDBError.SDB_FNE.getErrorCode()) {
                    throw e;
                }
            }
        }

        void truncateLob(DBCollection cl) throws InterruptedException {
            // waite for lob create
            Thread.sleep(5);

            try {
                cl.truncateLob(oid, data.length);
            } catch (BaseException e) {
                if (e.getErrorCode() != SDBError.SDB_LOB_IS_IN_USE.getErrorCode() &&
                        e.getErrorCode() != SDBError.SDB_FNE.getErrorCode()) {
                    throw e;
                }
            }
        }

        void removeLob(DBCollection cl) throws InterruptedException {
            // waite for lob create
            Thread.sleep(5);

            try {
                cl.removeLob(oid);
            } catch (BaseException e) {
                if (e.getErrorCode() != SDBError.SDB_LOB_IS_IN_USE.getErrorCode() &&
                        e.getErrorCode() != SDBError.SDB_FNE.getErrorCode()) {
                    throw e;
                }
            }
        }
    }

    enum LobOpType {
        PUT,
        READ,
        WRITE,
        TRUNCATE,
        REMOVE
    }
}

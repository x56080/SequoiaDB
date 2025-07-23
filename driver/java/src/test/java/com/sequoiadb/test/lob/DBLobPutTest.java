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

   Source File Name = DBLobPutTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.lob;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.*;

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
        bigData = new byte[256 * 1024]; // 256KB

        fillData(normalData);
        fillData(bigData);
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

    private static void fillData(byte[] b) {
        for (int i = 0; i < b.length; i++) {
            b[i] = (byte)i;
        }
    }

    @Test
    public void normalTest() {
        ObjectId oid;

        // case 1: data is null, oid is null
        try {
            putAndCheck(null, null, 0, 0);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 2: empty data
        putAndCheck( null, emptyData, 0, emptyData.length);

        // case 3: normal data, oid is null
        putAndCheck(null, normalData, 0, normalData.length);

        // case 4: normal data and oid
        oid = cl.createLobID();
        putAndCheck(oid, normalData, 0, normalData.length);

        // case 5: oid repeat
        try {
            putAndCheck(oid, normalData, 0, normalData.length);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_FE.getErrorCode(), e.getErrorCode());
        }

        // case 6: big lob
        putAndCheck(null, bigData, 0, bigData.length);

        // case 7: offset < 0
        try {
            putAndCheck(null, normalData, -1, normalData.length);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 8: offset = data length
        putAndCheck(null, normalData, normalData.length, 0);

        // case 9: offset > data length
        try {
            putAndCheck(null, normalData, normalData.length + 1, 0);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 10: len < 0
        try {
            putAndCheck(null, normalData, 0, -1);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 11: len = 0
        putAndCheck(null, normalData, 0, 0);

        // case 12: len > data length
        try {
            putAndCheck(null, normalData, 0, normalData.length + 1);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 13: offset + len < data length
        putAndCheck(null, normalData, 1, normalData.length - 1);

        // case 14: offset + len > data length
        try {
            putAndCheck(null, normalData, 1, normalData.length);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case 15: bigdata with offset and len
        putAndCheck(null, bigData, 1, bigData.length - 1 );
    }

    private void putAndCheck(ObjectId oid, byte[] data, int offset, int len) {
        if (oid != null) {
            cl.putLob(data, offset, len, oid);
        } else {
            oid = cl.putLob(data, offset, len);
        }
        byte[] exceptData = Arrays.copyOfRange(data, offset, offset + len);
        checkLobData(oid, exceptData);
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
            cl.putLob(data, oid);
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

package com.sequoiadb.lob;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:seqDB-10425:并发创建/读取/删除lob 多个客户端并发执行插入、读取、删除lob操作
 * @Author linsuqiang
 * @Date 2016-12-12
 * @Version 1.00
 */
public class TestLob10425 extends SdbTestBase {
    private String clName = "cl_10425";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private List<ObjectId> readOids = new ArrayList<ObjectId>();
    private List<ObjectId> removeOids = new ArrayList<ObjectId>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        DBCollection cl = createCL();
        String[] lobStrs = buildLobStrs(100);
        putLobs(cl, lobStrs);
    }

    @AfterClass
    public void tearDown() {
        try {
            if (cs.isCollectionExist(clName)) {
                cs.dropCollection(clName);
            }
            sdb.disconnect();
        } finally {
            sdb.disconnect();
        }
    }

    @Test
    public void test() {
        PutLobsThread putLobsThread = new PutLobsThread();
        ReadLobsThread readLobsThread = new ReadLobsThread(readOids);
        RemoveLobsThread removeLobsThread = new RemoveLobsThread(removeOids);

        putLobsThread.start(5);
        readLobsThread.start(5);
        removeLobsThread.start(5);

        if (!(putLobsThread.isSuccess() && readLobsThread.isSuccess() && removeLobsThread.isSuccess())) {
            Assert.fail(putLobsThread.getErrorMsg() + readLobsThread.getErrorMsg() + removeLobsThread.getErrorMsg());
        }
    }

    private class PutLobsThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
                cl = db.getCollectionSpace(csName).getCollection(clName);
                // do put lobs
                String[] lobStrs = buildLobStrs(10);
                for (int i = 0; i < lobStrs.length; i++) {
                    DBLob lob = null;
                    try {
                        lob = cl.createLob();
                        lob.write(lobStrs[i].getBytes());
                    } catch (BaseException e) {
                        throw e;
                    } finally {
                        if (lob != null) {
                            lob.close();
                        }
                    }
                }
            } finally {
                db.disconnect();
            }
        }
    }

    private class ReadLobsThread extends SdbThreadBase {
        private List<ObjectId> oids = null;

        public ReadLobsThread(List<ObjectId> oids) {
            this.oids = oids;
        }

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
                db.setSessionAttr(new BasicBSONObject("PreferedInstance", "M"));
                cl = db.getCollectionSpace(csName).getCollection(clName);
                // do read lobs
                for (ObjectId oid : oids) {
                    DBLob lob = cl.openLob(oid);
                    byte[] buff = new byte[(int) lob.getSize()];
                    lob.read(buff);
                    lob.close();
                }
            } finally {
                db.disconnect();
            }
        }
    }

    private class RemoveLobsThread extends SdbThreadBase {
        private List<ObjectId> oids = null;

        public RemoveLobsThread(List<ObjectId> oids) {
            this.oids = oids;
        }

        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
                cl = db.getCollectionSpace(csName).getCollection(clName);
                // do remove lobs
                for (ObjectId oid : oids) {
                    cl.removeLob(oid);
                }
            } finally {
                db.disconnect();
            }
        }

    }

    private DBCollection createCL() {
        try {
            if (!sdb.isCollectionSpaceExist(SdbTestBase.csName)) {
                sdb.createCollectionSpace(SdbTestBase.csName);
            }
        } catch (BaseException e) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals(-33, e.getErrorCode(), e.getMessage());
        }
        DBCollection cl = null;
        cs = sdb.getCollectionSpace(SdbTestBase.csName);
        BSONObject options = new BasicBSONObject();
        options = (BSONObject) JSON.parse("{ShardingKey:{a:1,b:-1},ShardingType:'hash',Partition:4096, ReplSize:0}");
        cl = cs.createCollection(clName, options);
        return cl;
    }

    private String[] buildLobStrs(int lobSum) {
        // build lobs
        String[] lobStrs = new String[lobSum];
        Random random = new Random();
        for (int i = 0; i < lobStrs.length; i++) {
            int lobsize = random.nextInt(1048576);
            lobStrs[i] = LobOprUtils.getRandomString(lobsize);
        }
        return lobStrs;
    }

    private void putLobs(DBCollection cl, String lobStrs[]) {
        for (int i = 0; i < lobStrs.length; i++) {
            DBLob lob = null;
            try {
                lob = cl.createLob();
                lob.write(lobStrs[i].getBytes());
            } finally {
                if (lob != null) {
                    lob.close();
                }
            }
        }
    }
}
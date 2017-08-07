package com.sequoiadb.datasync.brokennetwork;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-2936: 删除索引过程中主节点断网，该主节点为同步的源节点
 * @Author linsuqiang
 * @Date 2017-03-27
 * @Version 1.00
 */

/*
 * 1.创建CS，CL 
 * 2.批量创建索引 
 * 3.批量删除创建的索引 
 * 4.过程中构造断网故障(例如：ifdown) 
 * 5.选主成功后，继续删除 
 * 6.过程中故障恢复 (例如：ifup)，验证索引信息 
 */

public class DropIndex2936 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clNameBase = "cl_2936";
    private String clGroupName = null;
    private static int CL_NUM = 100;
    private static int IDX_NUM = 60;
    private GroupWrapper dataGroup = null;
    private String dataPriHost = null;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {
            System.out.println("the TestCase Name:" + this.getClass().getName() + ". the TestCase begin at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));

            groupMgr = new GroupMgr();
            if (!groupMgr.checkBusiness()) {
                throw new SkipException("checkBusiness failed");
            }

            clGroupName = groupMgr.getAllDataGroupName().get(0);
            GroupWrapper cataGroup = groupMgr.getGroupByName("SYSCatalogGroup");
            String cataPriHost = cataGroup.getMaster().hostName();
            dataGroup = groupMgr.getGroupByName(clGroupName);
            dataPriHost = dataGroup.getMaster().hostName();
            if (cataPriHost.equals(dataPriHost) && !cataGroup.changePrimary()) {
                throw new SkipException(cataGroup.getGroupName() + " reelect fail");
            }

            db = new Sequoiadb(coordUrl, "", "");
            createCLs(db);
            createIndexes(db);
        } catch (ReliabilityException e) {
            Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage() + "\r\n"
                    + Utils.getKeyStack(e, this));
        } finally {
            if (db != null) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            FaultMakeTask faultTask = BrokenNetwork.getFaultMakeTask(dataPriHost, 1, 10);
            TaskMgr mgr = new TaskMgr(faultTask);
            String safeUrl = CommLib.getSafeCoordUrl(dataPriHost);
            DropIdxTask dTask = new DropIdxTask(safeUrl);
            mgr.addTask(dTask);
            mgr.execute();
            Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());

            if (!groupMgr.checkBusinessWithLSN(600)) {
                Assert.fail("checkBusinessWithLSN() occurs timeout");
            }

            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            checkConsistency(dataGroup);
            runSuccess = true;
        } catch (ReliabilityException e) {
            e.printStackTrace();
            Assert.fail(e.getMessage());
        } finally {
            if (db != null) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if (!runSuccess) {
            throw new SkipException("to save environment");
        }
        Sequoiadb db = null;
        try {
            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            dropCLs(db);
        } catch (BaseException e) {
            Assert.fail(e.getMessage() + "\r\n" + Utils.getKeyStack(e, this));
        } finally {
            if (db != null) {
                db.close();
            }
            System.out.println("the TestCase Name:" + this.getClass().getName() + ". the TestCase end at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        }
    }

    private void createCLs(Sequoiadb db) {
        CollectionSpace commCS = db.getCollectionSpace(csName);
        for (int i = 0; i < CL_NUM; i++) {
            String clName = clNameBase + "_" + i;
            BSONObject option = (BSONObject) JSON.parse("{ Group: '" + clGroupName + "', ReplSize: 1 }");
            commCS.createCollection(clName, option);
        }
    }

    private void createIndexes(Sequoiadb db) {
        CollectionSpace commCS = db.getCollectionSpace(csName);
        for (int i = 0; i < CL_NUM; i++) {
            String clName = clNameBase + "_" + i;
            DBCollection cl = commCS.getCollection(clName);
            for (int j = 0; j < IDX_NUM; j++) {
                String idxName = "idx_" + j;
                BSONObject key = (BSONObject) JSON.parse("{ a" + j + ": 1 }");
                boolean isUnique = j % 2 == 0 ? true : false;
                boolean enforced = false;
                int sortBufferSize = j * 2;
                cl.createIndex(idxName, key, isUnique, enforced, sortBufferSize);
            }
        }
    }

    private void dropCLs(Sequoiadb db) {
        CollectionSpace commCS = db.getCollectionSpace(csName);
        for (int i = 0; i < CL_NUM; i++) {
            String clName = clNameBase + "_" + i;
            commCS.dropCollection(clName);
        }
    }

    private class DropIdxTask extends OperateTask {
        private String safeUrl = null;

        public DropIdxTask(String safeUrl) {
            this.safeUrl = safeUrl;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb(safeUrl, "", "");
                CollectionSpace commCS = db.getCollectionSpace(csName);
                for (int i = 0; i < CL_NUM; i++) {
                    String clName = clNameBase + "_" + i;
                    DBCollection cl = commCS.getCollection(clName);
                    for (int j = 0; j < IDX_NUM; j++) {
                        String idxName = "idx_" + j;
                        try {
                            cl.dropIndex(idxName);
                        } catch (BaseException e) {
                        }
                    }
                }
            } catch (BaseException e) {
            } finally {
                if (db != null) {
                    db.close();
                }
            }
        }
    }

    private void checkConsistency(GroupWrapper dataGroup) {
        boolean checkOk = false;
        int checkTimes = 30;
        int checkInterval = 1000; // 1s
        String lastCompareInfo = "";
        for (int j = 0; j < checkTimes; j++) {
            List<String> dataUrls = dataGroup.getAllUrls();
            List<List<BSONObject>> results = new ArrayList<List<BSONObject>>();
            for (String dataUrl : dataUrls) {
                Sequoiadb dataDB = new Sequoiadb(dataUrl, "", "");
                List<BSONObject> result = new ArrayList<BSONObject>();
                for (int i = 0; i < CL_NUM; i++) {
                    String clName = clNameBase + "_" + i;
                    DBCollection cl = dataDB.getCollectionSpace(csName).getCollection(clName);
                    DBCursor cursor = cl.getIndexes();
                    while (cursor.hasNext()) {
                        result.add(cursor.getNext());
                    }
                    cursor.close();
                }
                results.add(result);
                dataDB.close();
            }

            List<BSONObject> compareA = results.get(0);
            sortByName(compareA);
            removeUnconcerned(compareA);
            checkOk = true;
            for (int i = 1; i < results.size(); i++) {
                List<BSONObject> compareB = results.get(i);
                sortByName(compareB);
                removeUnconcerned(compareB);
                if (!compareA.equals(compareB)) {
                    lastCompareInfo = "";
                    lastCompareInfo += dataUrls.get(0) + "\n";
                    lastCompareInfo += compareA + "\n";
                    lastCompareInfo += dataUrls.get(i) + "\n";
                    lastCompareInfo += compareB + "\n";
                    checkOk = false;
                }
            }

            if (checkOk) {
                break;
            }

            try {
                Thread.sleep(checkInterval);
            } catch (InterruptedException e) {
                // ignore
            }
        }

        if (!checkOk) {
            System.out.println(lastCompareInfo);
            Assert.fail("data is different. see the detail in console");
        }
    }

    private void sortByName(List<BSONObject> list) {
        Collections.sort(list, new Comparator<BSONObject>() {
            public int compare(BSONObject a, BSONObject b) {
                String aName = (String) ((BSONObject) a.get("IndexDef")).get("name");
                String bName = (String) ((BSONObject) b.get("IndexDef")).get("name");
                return aName.compareTo(bName);
            }
        });
    }

    private void removeUnconcerned(List<BSONObject> list) {
        for (BSONObject obj : list) {
            obj.removeField("IndexFlag");
            ((BSONObject) obj.get("IndexDef")).removeField("_id");
        }
    }
}
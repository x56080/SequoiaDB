package com.sequoiadb.lob;

import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.types.ObjectId;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.sequoiadb.metaopr.commons.MyUtil.createRandomBytes;
import static org.testng.Assert.assertTrue;

/**
 * @FileName 测试用例 seqDB-3885 :: 版本: 2 :: 切分表写lob时备节点正常重启
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 */
public class Lob3885 implements StandTestInterface {
    private String csName = LobUtil.csName;
    private String clName = LobUtil.clName;

    private final byte[] data = createRandomBytes(200 * 1024);
    private List<ObjectId> objectIds = new ArrayList<>();
    private List<byte[]> byteList = new ArrayList<>();


    /**
     * 1.在集合上并发执行以下操作：
     * (1)不停写Lob，覆盖不同lob大小，例如：200k、500M ;
     * (2)不停地删除，并发操作时，备节点正常重启
     * 2.故障恢复后，再次进行写/删除lob
     *
     * @throws ReliabilityException
     */
    @Test
    public void test() throws ReliabilityException {
        //备节点重启
        GroupMgr groupMgr = new GroupMgr();
        NodeWrapper node = groupMgr.getGroupByName("group1").getSlave();
        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask(node, 0, 5);

        //并发读写lob
        List<ObjectId> createIds = new ArrayList<>(50);
        Map<ObjectId, String> deteledIdMap = new HashMap<>();

        LobTask createLobsTask = LobTask.getCreateLobsTask(1000, data, createIds);
        createLobsTask.setName("create lobs task");
        LobTask deleteLobsTask = LobTask.getDeleteLobsTask(objectIds, deteledIdMap);
        deleteLobsTask.setName("delete lobs task ");

        TaskMgr mgr = new TaskMgr(faultMakeTask);
        mgr.addTask(createLobsTask);
        mgr.addTask(deleteLobsTask);
        mgr.execute();

        byte[] targetMd5Value = MyUtil.getMd5(data);
        assertTrue(MyUtil.isLobsAllCreated(csName, clName, createIds));
        assertTrue(MyUtil.isLobsAllDelete(csName, clName, deteledIdMap));
        assertTrue(MyUtil.isLobNumInspectInGroup(csName, clName, "group1"));
        assertTrue(MyUtil.isLobNumInspectInGroup(csName, clName, "group2"));
        assertTrue(MyUtil.isLobMd5InspectInGroup(csName, clName, "group1", targetMd5Value));
        assertTrue(MyUtil.isLobMd5InspectInGroup(csName, clName, "group2", targetMd5Value));
    }

    private void createLob() {
        for (int i = 0; i < 500; i++) {
            objectIds.add(MyUtil.createLob(csName, clName, data));
        }
    }

    @BeforeClass
    @Override
    public void setup() {
        MyUtil.printBeginTime(this);
        LobUtil.createLobCsAndCl();
        MyUtil.deleteAllLobs(csName, clName);
        createLob();
    }


    @AfterClass
    @Override
    public void tearDown() {
        MyUtil.deleteAllLobs(csName, clName);
        LobUtil.dropLobCS();
        MyUtil.printEndTime(this);
    }
}

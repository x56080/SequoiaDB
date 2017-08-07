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

import static com.sequoiadb.metaopr.commons.MyUtil.createLob;
import static com.sequoiadb.metaopr.commons.MyUtil.createRandomBytes;
import static org.testng.AssertJUnit.assertTrue;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 */
public class Lob11434 implements StandTestInterface {
    String csName = LobUtil.csName;
    String clName = LobUtil.clName;
    private byte[] data = createRandomBytes(200 * 1024);
    private List<ObjectId> ids = new ArrayList<>();


    @BeforeClass
    @Override
    public void setup() {
        MyUtil.printBeginTime(this);
        LobUtil.createLobCsAndCl();
        MyUtil.deleteAllLobs(csName, clName);
        for (int i = 0; i < 100; i++) {
            ids.add(createLob(csName, clName, data));
        }
    }

    @AfterClass
    @Override
    public void tearDown() {
        LobUtil.dropLobCS();
        MyUtil.printEndTime(this);
    }

    /**
     * 1.在集合上并发执行以下操作：
     *      (1)循环增删改查数据、循环读写Lob，其中lob大小取不同值，如200K/500k
     *      (2)循环读写删lob、循环增删改查数据操作
     * 2.并发执行步骤1时，数据组主节点正常重启
     * 3.故障恢复后，再次读写lob操作
     *
     * @throws ReliabilityException
     */
    @Test
    public void test() throws ReliabilityException {
        GroupMgr groupMgr = new GroupMgr();
        NodeWrapper master = groupMgr.getGroupByName("group1").getMaster();
        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask(master, 0, 3);

        List<ObjectId> createdId = new ArrayList<>();
        HashMap<ObjectId, String> deletedIdMap = new HashMap<>();
        LobTask createTask = LobTask.getCreateLobsTask(100, data, createdId);
        createTask.setName("create lob task");
        LobTask readTask = LobTask.getReadLobsTask(ids);
        readTask.setName("read lob task");
        LobTask deleteTask = LobTask.getDeleteLobsTask(ids, deletedIdMap);
        deleteTask.setName("delete lob task");

        TaskMgr taskMgr = new TaskMgr(faultMakeTask);
        taskMgr.addTask(createTask);
        taskMgr.addTask(readTask);
        taskMgr.addTask(deleteTask);
        taskMgr.execute();

        byte[] targetMd5Value = MyUtil.getMd5(data);
        groupMgr.checkBusiness();
        assertTrue(MyUtil.isLobsAllCreated(csName, clName, createdId));
        assertTrue(MyUtil.isLobsAllDelete(csName, clName, deletedIdMap));
        assertTrue(MyUtil.isLobNumInspectInGroup(csName, clName, "group1"));
        assertTrue(MyUtil.isLobNumInspectInGroup(csName, clName, "group2"));
        assertTrue(MyUtil.isLobMd5InspectInGroup(csName, clName, "group1", targetMd5Value));
        assertTrue(MyUtil.isLobMd5InspectInGroup(csName, clName, "group2", targetMd5Value));
    }


}

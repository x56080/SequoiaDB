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
public class Lob3890 implements StandTestInterface {
    String csName = LobUtil.csName;
    String clName = LobUtil.clName;
    private byte[] data = createRandomBytes(200 * 1024);
    private List<ObjectId> ids = new ArrayList<>();
    private TaskMgr taskMgr = new TaskMgr();

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
        MyUtil.printEndTime(this);
        LobUtil.dropLobCS();
    }

    @Test
    public void test() throws ReliabilityException {
        allNodeRestart();

        List<ObjectId> createdId = new ArrayList<>();
        HashMap<ObjectId, String> deletedIdMap = new HashMap<>();

        LobTask createTask = LobTask.getCreateLobsTask(1000, data, createdId);
        createTask.setName("create lob task");
        LobTask readTask = LobTask.getReadLobsTask(ids);
        readTask.setName("read lob task ");
        LobTask deleteTask = LobTask.getDeleteLobsTask(ids, deletedIdMap);
        deleteTask.setName("delete lob task");

        taskMgr.addTask(createTask);
        taskMgr.addTask(readTask);
        taskMgr.addTask(deleteTask);

        taskMgr.execute();

        byte[] targetMd5Value = MyUtil.getMd5(data);
        assertTrue(MyUtil.isLobsAllCreated(csName, clName, createdId));
        assertTrue(MyUtil.isLobsAllDelete(csName, clName, deletedIdMap));
        assertTrue(MyUtil.isLobNumInspectInGroup(csName, clName, "group1"));
        assertTrue(MyUtil.isLobNumInspectInGroup(csName, clName, "group2"));
        assertTrue(MyUtil.isLobMd5InspectInGroup(csName, clName, "group1", targetMd5Value));
        assertTrue(MyUtil.isLobMd5InspectInGroup(csName, clName, "group2", targetMd5Value));
    }

    private void allNodeRestart() throws ReliabilityException {
        GroupMgr groupMgr = new GroupMgr();
        for (NodeWrapper node : groupMgr.getGroupByName("group1").getNodes()) {
            FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask(node, 0, 3);
            taskMgr.addTask(faultMakeTask);
        }
        for (NodeWrapper node : groupMgr.getGroupByName("group2").getNodes()) {
            FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask(node, 0, 3);
            taskMgr.addTask(faultMakeTask);
        }
    }
}

package com.sequoiadb.lob;

import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
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
public class Lob3476 implements StandTestInterface {
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
     * 1.在集合上并发执行以下操作： (1)不停地写lob，覆盖不同lob大小，例如：200k、500M (2)不停地读lob (3)不停地删除lob，并发的同时主节点断网
     * 2.成功选出新主后，继续读写删lob
     * 3.故障恢复后，检查lob数据
     *
     * @throws ReliabilityException
     */
    @Test
    public void test() throws ReliabilityException {

        GroupMgr groupMgr = new GroupMgr();
        String hostName = groupMgr.getGroupByName("group1").getMaster().hostName();
        FaultMakeTask faultMakeTask = BrokenNetwork.getFaultMakeTask(hostName, 0, 5);

        List<ObjectId> createdId = new ArrayList<>();
        HashMap<ObjectId, String> deletedIdMap = new HashMap<>();
        LobTask createTask = LobTask.getCreateLobsTask(1000, data, createdId);
        LobTask readTask = LobTask.getReadLobsTask(ids);
        LobTask deleteTask = LobTask.getDeleteLobsTask(ids, deletedIdMap);

        TaskMgr taskMgr = new TaskMgr(faultMakeTask);
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
}

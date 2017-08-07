package com.sequoiadb.lob;

import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
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
public class Lob11437 implements StandTestInterface {
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
     * 1.开启事务
     * 2.cl中执行增删改操作
     * 3.循环写入lob，过程中数据主节点所在主机断网（lob写入的数据组主节点）
     * 4、故障恢复后，检查操作结果
     */
    public void test(FaultMakeTask faultMakeTask) throws ReliabilityException {
        setup();

        OperateTask clTask = ClTask.getClTask(1000);
        List<ObjectId> createdLobIds = new ArrayList<>();
        LobTask lobTask = LobTask.getCreateLobsTask(1000, data, createdLobIds);

        TaskMgr taskMgr = new TaskMgr(faultMakeTask);
        taskMgr.addTask(lobTask);
        taskMgr.addTask(clTask);
        taskMgr.execute();

        byte[] targetMd5Value = MyUtil.getMd5(data);
        assertTrue(MyUtil.isLobsAllCreated(csName, clName, createdLobIds));
        assertTrue(MyUtil.isLobNumInspectInGroup(csName, clName, "group1"));
        assertTrue(MyUtil.isLobNumInspectInGroup(csName, clName, "group2"));
        assertTrue(MyUtil.isLobMd5InspectInGroup(csName, clName, "group1", targetMd5Value));
        assertTrue(MyUtil.isLobMd5InspectInGroup(csName, clName, "group2", targetMd5Value));

        Assert.assertTrue(isCountOfClInspect());
    }

    private boolean isCountOfClInspect() throws ReliabilityException {
        GroupMgr mgr = new GroupMgr();

        GroupWrapper groupWrapper = mgr.getGroupByName("group1");
        long num = MyUtil.getClCountFromNode(csName, clName, groupWrapper.getMaster());
        for (NodeWrapper node : groupWrapper.getNodes()) {
            long temp = MyUtil.getClCountFromNode(csName, clName, groupWrapper.getMaster());
            if (temp != num)
                return false;
        }

        groupWrapper = mgr.getGroupByName("group2");
        num = MyUtil.getClCountFromNode(csName, clName, groupWrapper.getMaster());
        for (NodeWrapper node : groupWrapper.getNodes()) {
            long temp = MyUtil.getClCountFromNode(csName, clName, groupWrapper.getMaster());
            if (temp != num)
                return false;
        }
        return true;
    }

    /**
     * 1.开启事务
     * 2.cl中执行增删改操作
     * 3.循环写入lob，过程中数据备节点所在主机断网（lob写入的数据组备节点）
     * 4、执行事务提交操作
     * 5、故障恢复后，检查操作结果
     *
     * @throws ReliabilityException
     */
    @Test
    public void testSlaver() throws ReliabilityException {
        GroupMgr mgr = new GroupMgr();
        NodeWrapper node = mgr.getGroupByName("group1").getSlave();
        test(BrokenNetwork.getFaultMakeTask(node.hostName(), 0, 5));
    }

    @Test
    public void testMaster() throws ReliabilityException {
        GroupMgr mgr = new GroupMgr();
        NodeWrapper node = mgr.getGroupByName("group1").getMaster();
        test(BrokenNetwork.getFaultMakeTask(node.hostName(), 0, 5));
    }
}

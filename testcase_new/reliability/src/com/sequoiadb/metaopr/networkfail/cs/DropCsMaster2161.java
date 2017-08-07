package com.sequoiadb.metaopr.networkfail.cs;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.metaopr.commons.DBoperateTask;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

import static com.sequoiadb.metaopr.commons.MyUtil.*;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-4-28
 * @Version 1.00
 */
public class DropCsMaster2161 implements StandTestInterface {
    List<String> csNames;

    @BeforeClass
    @Override
    public void setup() {
        printBeginTime(this);
        checkBusiness();
        csNames = createNames("cs2161", 1000);
    }

    @AfterClass
    @Override
    public void tearDown() {
        dropCS(csNames);
        printEndTime(this);
    }

    /**
     * seqDB-2161 :: 版本: 1 :: 删除CS时catalog主节点断网_rlb.netSplit.metaOpr.CS.003
     * <p>
     * 1、创建CS，构造脚本循环执行创建CS操作db.createCS（）
     * 2、执行删除CS操作（构造脚本循环执行删除CS操作）
     * 3、删除CS时catalog主节点所在主机网络中断（构造网络中断故障，如ifdown网卡）
     * 3、查看CS信息和catalog主节点状态
     * 4、恢复网络故障（如ifup启动网卡）
     * 5、再次执行删除CS操作
     * 6、查看CS信息（执行listCollections（）命令查看CS信息）
     * 7、查看catalog主备节点是否存在该CS相关信息
     */
    @Test
    public void test() throws ReliabilityException, InterruptedException {
        createCS(csNames);

        DBoperateTask task=DBoperateTask.getTaskDropCs(csNames);
        String hostName=getMasterNodeOfCatalog().hostName();
        task.setHostname(CommLib.getSafeCoordUrl(hostName));
        FaultMakeTask faultMakeTask= BrokenNetwork.getFaultMakeTask(hostName,0,5);
        TaskMgr mgr=new TaskMgr(faultMakeTask,task);
        mgr.execute();

        checkBusiness();
        if(hostName.equals(getMasterNodeOfCatalog().hostName()))
            Thread.sleep(5*60*1000+10*1000);
        int breakIndex=task.getBreakIndex();
        dropCS(csNames.subList(breakIndex,csNames.size()));
        assertEquals(dropCS(csNames),0);
        assertTrue(isCsAllDeleted(csNames));
        assertTrue(isCatalogGroupSync());
    }
}

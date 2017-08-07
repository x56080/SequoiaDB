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
public class DropCsSlaver2162 implements StandTestInterface {
    List<String> names;

    @BeforeClass
    @Override
    public void setup() {
        printBeginTime(this);
        checkBusiness();
        names = createNames("cs2162", 1000);
        createCS(names);
    }

    @AfterClass
    @Override
    public void tearDown() {
        dropCS(names);
        printEndTime(this);
    }

    /**
     * seqDB-2162 :: 版本: 1 :: 删除CS时catalog备节点断网_rlb.netSplit.metaOpr.CS.004
     * <p>
     * 1、创建CS，构造脚本循环执行创建CS操作db.createCS（）
     * 2、执行删除CS操作（构造脚本循环执行删除CS操作）
     * 3、删除CS时catalog备节点所在主机网络中断（构造网络中断故障，如ifdown网卡）
     * 3、查看CS信息和catalog主节点状态
     * 4、恢复网络故障（如ifup启动网卡）
     * 5、再次执行删除CS操作
     * 6、查看CS信息（执行listCollections（）命令查看CS信息）
     * 8、查看catalog主备节点是否存在该CS相关信息
     */
    @Test
    public void test() throws ReliabilityException {
        DBoperateTask task=DBoperateTask.getTaskDropCs(names);
        String hostname=getMasterNodeOfCatalog().hostName();
        task.setHostname(CommLib.getSafeCoordUrl(hostname));
        FaultMakeTask faultMakeTask= BrokenNetwork.getFaultMakeTask(hostname,0,5);
        TaskMgr mgr= new TaskMgr(faultMakeTask,task);
        mgr.execute();

        checkBusiness();
        dropCS(names.subList(task.getBreakIndex(),names.size()));
        assertEquals(dropCS(names),0);
        assertTrue(isCsAllDeleted(names));
        assertTrue(isCatalogGroupSync());
    }
}

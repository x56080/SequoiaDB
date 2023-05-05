package org.sequoiadb.tool;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.junit.*;

import java.io.BufferedWriter;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;

public class JobMgrTest {
    private static SequoiadbDatasource datasource;
    private static final String jobFileName = "test_job.list";
    private static final String jobFileName2 = "test_job2.list";

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // create connection pool
        List<String> addrList = new ArrayList<>();
        addrList.add(Constants.COOR_NODE_CONN);
        // datasource option
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMaxCount(100);
        dsOpt.setMaxIdleCount(20);
        dsOpt.setMinIdleCount(10);
        dsOpt.setValidateConnection(true);
        // network option
        ConfigOptions nwOpt = new ConfigOptions();
        nwOpt.setConnectTimeout(200);
        nwOpt.setMaxAutoConnectRetryTime(0);
        datasource = new SequoiadbDatasource(addrList, "", "", nwOpt, dsOpt);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        datasource.close();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    @Ignore
    public void flushFileTest() {
        String fullName = "meta.file_2020";
        List<String> clList = new ArrayList<>();
        clList.add(fullName);
        JobMgr jobMgr = new JobMgr(datasource, jobFileName);
        jobMgr.initJobFile(clList);

        jobMgr.loadJobRecords();
        JobInfo jobInfo = jobMgr.getJobRecord();
        jobInfo.setCollectionName("a.a");
        jobMgr.updateJobRecord(jobInfo);
    }

    @Test
    @Ignore
    public void flushFileTest2() {
        /**
         * 1. write a log to jobFileName2
         * 2. and then test(by using little string, like "abc")
         * 3. "abc" will overwrite all the contents write before
         */
        try {
            BufferedWriter writer = new BufferedWriter(new FileWriter(jobFileName2));
            writer.write("abc");
            writer.newLine();
            writer.flush();
            writer.close();
        } catch (FileNotFoundException e) {
            String errMsg = "Job File " + jobFileName2 + " does not exist";
            throw new BaseException(SDBError.SDB_INVALIDARG, errMsg, e);
        } catch (Exception e) {
            String errMsg = "Failed to flush job records to file: " + jobFileName2;
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }
    }
}

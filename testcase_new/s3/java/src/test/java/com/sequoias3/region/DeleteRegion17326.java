package com.sequoias3.region;

import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;

/**
 * @Description seqDB-17326: 删除不存在的区域
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class DeleteRegion17326 extends S3TestBase {
    private String NonexistentRegion = "nonexistent17326";
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        regionClient = CommLib.regionClient();
    }

    @Test
    public void testGetRegionMessage() throws Exception {
        try {
            regionClient.deleteRegion(NonexistentRegion);
        } catch (SequoiaS3ServiceException e) {
            if (e.getStatusCode() != 404) {
                throw e;
            }
        }
    }

    @AfterClass
    private void tearDown() throws Exception {
        regionClient.shutdown();
    }
}

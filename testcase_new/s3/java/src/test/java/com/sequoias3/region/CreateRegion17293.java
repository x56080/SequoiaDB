package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17293: create Region and specify cs and cl. the cs and cl not exist
 * @author wuyan
 * @Date 2019.1.18
 * @version 1.00
 */
public class CreateRegion17293 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17293";
    private String[] csNames = { "metaCS17293", "dataCS17293" };
    private String[] metaclNames = { "metaCL17293", "metaHistroyCL17293" };
    private String[] dataclNames = { "dataCL17293" };
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL(csNames[0], metaclNames);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    public void testRegion() throws Exception {
        try {
            String metaLocation = csNames[0] + "." + metaclNames[0];
            String metaHisLocation = csNames[0] + "." + metaclNames[1];
            String dataLocation = csNames[1] + "." + dataclNames[0];
            CreateRegionRequest request = new CreateRegionRequest(regionName);
            request.withMetaLocation(metaLocation).withMetaHisLocation(metaHisLocation).withDataLocation(dataLocation);
            regionClient.createRegion(request);
            Assert.fail("put region must be fail!");
        } catch (SequoiaS3ServiceException e) {
            // return 400:InvalidLocation
            Assert.assertEquals(e.getStatusCode(), 400);
            Assert.assertEquals(e.getErrorCode(), "InvalidLocation");
        }

        Assert.assertFalse(regionClient.headRegion(regionName), "region should be not exist!");
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if (runSuccess) {
                RegionUtils.dropCS(csNames);
            }
        } finally {
            regionClient.shutdown();
        }
    }
}

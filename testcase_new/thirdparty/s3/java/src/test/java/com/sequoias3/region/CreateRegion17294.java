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
 * @Description seqDB-17294: create Region and specify cs and cl. the metaCL is the same as
 *              metaHisCL
 * @author wuyan
 * @Date 2019.1.21
 * @version 1.00
 */
public class CreateRegion17294 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17294";
    private String[] csNames = { "metaCS17294", "dataCS17294" };
    private String[] metaclNames = { "metaCL17294" };
    private String[] dataclNames = { "dataCL17294" };
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL(csNames[0], metaclNames);
        RegionUtils.createCSAndCL(csNames[1], dataclNames);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    public void testRegion() throws Exception {
        try {

            String metaLocation = csNames[0] + "." + metaclNames[0];
            String metaHisLocation = csNames[0] + "." + metaclNames[0];
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

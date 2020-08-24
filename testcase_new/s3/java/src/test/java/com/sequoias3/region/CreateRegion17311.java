package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17311: create Region and specify all config.
 * @author wuyan
 * @Date 2019.1.22
 * @version 1.00
 */
public class CreateRegion17311 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17311";
    private String[] csNames = { "metaCS17311", "dataCS17311" };
    private String[] metaclNames = { "metaCL17311", "metaHistroyCL17311" };
    private String[] dataclNames = { "dataCL17311" };
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
        RegionUtils.createCSAndCL(csNames[0], metaclNames);
        RegionUtils.createCSAndCL(csNames[1], dataclNames);
    }

    @Test
    public void testRegion() throws Exception {
        try {

            String metaLocation = csNames[0] + "." + metaclNames[0];
            String metaHisLocation = csNames[0] + "." + metaclNames[1];
            String dataLocation = csNames[1] + "." + dataclNames[0];
            CreateRegionRequest request = new CreateRegionRequest(regionName);
            request.withMetaLocation(metaLocation).withMetaHisLocation(metaHisLocation).withDataLocation(dataLocation)
                    .withDataCLShardingType(DataShardingType.MONTH).withDataCSShardingType(DataShardingType.YEAR);
            regionClient.createRegion(request);
            Assert.fail("put region must be fail!");
        } catch (SequoiaS3ServiceException e) {
            // return 409:ConflictRegionType
            Assert.assertEquals(e.getStatusCode(), 409);
            Assert.assertEquals(e.getErrorCode(), "ConflictRegionType");
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

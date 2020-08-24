package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-22093:指定模式配置DataLobPageSize和DataReplSize，创建/更新区域
 * @author fanyu
 * @Date:2019年04月21日
 * @version:1.0
 */
public class CuRegion22093 extends S3TestBase {
    private boolean runSuccess1 = false;
    private boolean runSuccess2 = false;
    private String regionNameBase = "region22093";
    private String[] csNames = { "metaCS22093", "dataCS22093" };
    private String[] metaclNames = { "metaCL22093", "metaHistroyCL22093" };
    private String[] dataclNames = { "dataCL22093" };
    private AmazonS3 s3Client = null;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        regionClient = CommLib.regionClient();
        RegionUtils.createCSAndCL(csNames[0], metaclNames);
        RegionUtils.createCSAndCL(csNames[1], dataclNames);
    }

    @Test
    private void test1() throws Exception {
        String metaLocation = csNames[0] + "." + metaclNames[0];
        String metaHisLocation = csNames[0] + "." + metaclNames[1];
        String dataLocation = csNames[1] + "." + dataclNames[0];
        CreateRegionRequest request = new CreateRegionRequest(regionNameBase + "A");
        request.withMetaLocation(metaLocation).withDataLocation(dataLocation).withMetaHisLocation(metaHisLocation)
                .withDataLobPageSize(262144).withDataReplSize(-1);
        try {
            regionClient.createRegion(request);
            Assert.fail("exp fail but act success!!!");
        } catch (SequoiaS3ServiceException e) {
            if (e.getStatusCode() != 409) {
                throw e;
            }
        }
        runSuccess1 = true;
    }

    @Test
    private void test2() throws Exception {
        String regionName = regionNameBase + "B";
        String metaLocation = csNames[0] + "." + metaclNames[0];
        String metaHisLocation = csNames[0] + "." + metaclNames[1];
        String dataLocation = csNames[1] + "." + dataclNames[0];
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withMetaLocation(metaLocation).withDataLocation(dataLocation).withMetaHisLocation(metaHisLocation);
        regionClient.createRegion(request);

        // update region
        request.withDataLobPageSize(262144).withDataReplSize(-1);
        try {
            regionClient.createRegion(request);
            Assert.fail("exp fail but act success!!!");
        } catch (SequoiaS3ServiceException e) {
            if (e.getStatusCode() != 409) {
                throw e;
            }
        }

        RegionUtils.clearRegion(regionClient, regionName);
        runSuccess2 = true;
    }

    @AfterClass
    private void tearDown() {
        if (runSuccess1 && runSuccess2) {
            regionClient.shutdown();
            if (s3Client != null) {
                s3Client.shutdown();
            }
            RegionUtils.dropCS(csNames);
        }
    }
}

package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ClientException;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17302 :: 更新区域使用指定模式，指定配置不一致
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class UpdateRegion17302 extends S3TestBase {
    private String regionName = "region17302";
    private String csName = "cs17302";
    private String[] clNames = { "dataCL17302A", "metaCL17302A", "metaHisCL17302A", "dataCL17302B", "metaCL17302B",
            "metaHisCL17302B" };
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL(csName, clNames);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    private void test() throws Exception {
        // create region
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withDataLocation(csName + "." + clNames[0]).withMetaLocation(csName + "." + clNames[1])
                .withMetaHisLocation(csName + "." + clNames[2]);
        regionClient.createRegion(request);

        // create region again by different Meta and Data location
        try {
            request.withDataCSShardingType(DataShardingType.YEAR).withDataCLShardingType(DataShardingType.YEAR)
                    .withDataLocation(csName + "." + clNames[3]).withMetaLocation(csName + "." + clNames[4])
                    .withMetaHisLocation(csName + "." + clNames[5]);
            regionClient.createRegion(request);
            Assert.fail("put region must be failed, region = " + request.toString());
        } catch (SequoiaS3ServiceException e) {
            if (e.getStatusCode() != 409 && !e.getErrorCode().contains("ConflictRegionType")) {
                throw e;
            }
        }

        // check region info
        checkGetRegionResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                RegionUtils.dropCS(new String[] { csName });
                regionClient.deleteRegion(regionName);
            }
        } finally {
            regionClient.shutdown();
        }

    }

    private void checkGetRegionResult() throws SequoiaS3ServiceException, SequoiaS3ClientException {
        GetRegionResult result = regionClient.getRegion(regionName);
        Region actRegion = result.getRegion();
        Assert.assertEquals(actRegion.getDataLocation(), csName + "." + clNames[0]);
        Assert.assertEquals(actRegion.getMetaLocation(), csName + "." + clNames[1]);
        Assert.assertEquals(actRegion.getMetaHisLocation(), csName + "." + clNames[2]);
        Assert.assertEquals(result.getBuckets().size(), 0);
    }
}

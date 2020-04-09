package com.sequoias3.region;

import com.amazonaws.services.s3.model.Bucket;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 获取空区域信息 testlink-case: seqDB-17321
 *
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class GetRegionMessage17321 extends S3TestBase {
    private String regionName = "beijing17321";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.clearRegion( regionName );
    }

    @Test
    public void testGetRegionMessage() throws Exception {
        // create region
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );

        GetRegionResult result = RegionUtils.getRegion( regionName );
        List< Bucket > bucketList = result.getBuckets();
        Assert.assertEquals( bucketList.size(), 0 );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            RegionUtils.deleteRegion( regionName );
        }
    }
}

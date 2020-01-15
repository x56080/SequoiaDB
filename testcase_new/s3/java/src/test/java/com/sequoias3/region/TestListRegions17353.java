package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: ListRegions接口参数校验 testlink-case: seqDB-17353
 *
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class TestListRegions17353 extends S3TestBase {
    private String regionName = "Beijing17353";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.clearRegion( regionName );
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // 合法值
        List<String> regions = RegionUtils.listRegions();
        Assert.assertTrue( regions.contains( regionName.toLowerCase() ) );

        // 非法值
        try {
            RegionUtils.listRegions( "" );
            Assert.fail( "list regions with '' should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidAccessKeyId" );
        }

        try {
            RegionUtils.listRegions( new String() );
            Assert.fail( "list regions with null should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidAccessKeyId" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            RegionUtils.deleteRegion( regionName );
        }
    }
}

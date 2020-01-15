package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 非管理员用户使用head查询区域 testlink-case: seqDB-17329
 *
 * @author wangkexin
 * @Date 2019.01.25
 * @version 1.00
 */

public class HeadRegionListByNormalUser17329 extends S3TestBase {
    private String userName = "user17329";
    private String roleName = "normal";
    private String[] accessKeys = null;
    private String regionName = "Beijing17329";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // create regions
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );

        try {
            RegionUtils.headRegion( regionName, accessKeys[ 0 ] );
            Assert.fail( "Non-Administrator user head regions should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 403 );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            RegionUtils.deleteRegion( regionName );
            UserUtils.deleteUser( userName );
        }
    }
}

package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.base.Sequoiadb;
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
 * test content: 非管理员用户创建区域 testlink-case: seqDB-17309
 *
 * @author wangkexin
 * @Date 2019.01.23
 * @version 1.00
 */

public class CreateRegionByNormalUser17309 extends S3TestBase {
    private String userName = "user17309";
    private String roleName = "normal";
    private String[] accessKeys = null;
    private String regionName = "Beijing17309";
    private String metaCSName = "metaCS17309";
    private String dataCSName = "dataCS17309";
    private String[] metaClNames = { "metaCL17309", "metaHistoryCL17309" };
    private String[] dataClName = { "dataCL17309" };
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        RegionUtils.createCSAndCL( metaCSName, metaClNames );
        RegionUtils.createCSAndCL( dataCSName, dataClName );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // create region
        Region region = new Region();
        region.withName( regionName )
                .withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
        try {
            RegionUtils.putRegion( region, accessKeys[ 0 ] );
            Assert.fail( "Non-Administrator user put region should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        Assert.assertFalse( RegionUtils.headRegion( regionName ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "",
                    "" )) {
                sdb.dropCollectionSpace( metaCSName );
                sdb.dropCollectionSpace( dataCSName );
                UserUtils.deleteUser( userName );
            }
        }
    }
}

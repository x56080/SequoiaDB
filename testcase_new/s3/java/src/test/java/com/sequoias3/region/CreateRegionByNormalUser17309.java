package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;

/**
 * @Description seqDB-17309: 非管理员用户创建区域
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
    private SequoiaS3 regionClient = null;
    private SequoiaS3 regionClientM = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        RegionUtils.createCSAndCL( metaCSName, metaClNames );
        RegionUtils.createCSAndCL( dataCSName, dataClName );
        regionClientM = CommLib.regionClient();
        RegionUtils.clearRegion( regionClientM, regionName );
        regionClient = CommLib.regionClient( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // create region
        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
        try {
            regionClient.createRegion( request );
            Assert.fail( "Non-Administrator user put region should fail" );
        } catch ( SequoiaS3ServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        Assert.assertFalse( regionClientM.headRegion( regionName ) );
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
            } finally {
                regionClient.shutdown();
                regionClientM.shutdown();
            }
        }
    }
}

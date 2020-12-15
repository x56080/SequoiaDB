package com.sequoias3.region.concurrent;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17337:并发删除区域
 * @author wangkexin
 * @Date 2019.01.29
 * @version 1.00
 */

public class DeleteSameRegion17337 extends S3TestBase {
    private String regionName = "Beijing17337";
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );
        regionClient.createRegion( regionName );
    }

    @Test
    public void testDeleteRegion() throws Exception {
        DeleteRegionThread deleteRegionThread = new DeleteRegionThread();
        deleteRegionThread.start( 100 );
        Assert.assertTrue( deleteRegionThread.isSuccess() );
        Assert.assertFalse( regionClient.headRegion( regionName ) );
    }

    @AfterClass
    private void tearDown() throws Exception {
        regionClient.shutdown();
    }

    private class DeleteRegionThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            SequoiaS3 regionClientNew = CommLib.regionClient();
            ;
            try {
                regionClient.deleteRegion( regionName );
            } catch ( SequoiaS3ServiceException e ) {
                if ( e.getStatusCode() != 404 ) {
                    throw e;
                }
            } finally {
                regionClientNew.shutdown();
            }
        }
    }
}

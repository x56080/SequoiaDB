package com.sequoias3.region.concurrent;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 并发删除区域 testlink-case: seqDB-17337
 *
 * @author wangkexin
 * @Date 2019.01.29
 * @version 1.00
 */

public class DeleteSameRegion17337 extends S3TestBase {
    private String regionName = "Beijing17337";

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.clearRegion( regionName );
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );
    }

    @Test
    public void testDeleteRegion() throws Exception {
        DeleteRegionThread deleteRegionThread = new DeleteRegionThread();
        deleteRegionThread.start( 100 );
        Assert.assertTrue( deleteRegionThread.isSuccess() );
        Assert.assertFalse( RegionUtils.headRegion( regionName ) );
    }

    @AfterClass
    private void tearDown() throws Exception {
    }

    private class DeleteRegionThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            try {
                RegionUtils.deleteRegion( regionName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 404 ) {
                    throw e;
                }
            }
        }
    }
}

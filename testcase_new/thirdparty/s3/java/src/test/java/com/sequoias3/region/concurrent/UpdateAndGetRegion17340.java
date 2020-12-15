package com.sequoias3.region.concurrent;

import java.io.File;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17340: concurrent update region and get region.
 * @author wuyan
 * @Date 2019.1.31
 * @version 1.00
 */
public class UpdateAndGetRegion17340 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket17340";
    private String key = "key17340";
    private String regionName = "region17340";
    private AmazonS3 s3Client = null;
    private DataShardingType oldShardingType = DataShardingType.YEAR;
    private DataShardingType newShardingType = DataShardingType.MONTH;
    private int fileSize = 1024 * 20;
    private File localPath = null;
    private String filePath = null;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );

        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withDataCSShardingType( oldShardingType )
                .withDataCLShardingType( oldShardingType );
        regionClient.createRegion( request );
    }

    @Test
    public void testRegion() throws Exception {
        UpdateRegion updateRegion = new UpdateRegion();
        GetRegion getRegion = new GetRegion();
        updateRegion.start( 10 );
        getRegion.start( 10 );
        Assert.assertTrue( updateRegion.isSuccess(),
                updateRegion.getErrorMsg() );
        Assert.assertTrue( getRegion.isSuccess(), getRegion.getErrorMsg() );
        checkResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                regionClient.deleteRegion( regionName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            regionClient.shutdown();
            s3Client.shutdown();
        }
    }

    @SuppressWarnings("deprecation")
    private void checkResult() throws Exception {
        Assert.assertTrue( regionClient.headRegion( regionName ) );

        // create bucket and object on region
        s3Client.createBucket( bucketName, regionName );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private class UpdateRegion extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            SequoiaS3 regionClientNew = CommLib.regionClient();
            CreateRegionRequest request = new CreateRegionRequest( regionName );
            request.withDataCSShardingType( newShardingType )
                    .withDataCLShardingType( newShardingType );
            regionClientNew.createRegion( request );
            regionClientNew.shutdown();
        }
    }

    private class GetRegion extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            SequoiaS3 regionClientNew = CommLib.regionClient();
            GetRegionResult result = regionClientNew.getRegion( regionName );
            Region regionInfo = result.getRegion();
            DataShardingType dataCLShardingType = regionInfo
                    .getDataCLShardingType();
            if ( dataCLShardingType.equals( newShardingType ) ) {
                Assert.assertEquals( regionInfo.getDataCSShardingType(),
                        newShardingType );
            } else {
                Assert.assertEquals( regionInfo.getDataCSShardingType(),
                        oldShardingType );
            }
            // get the region infor to take the default value
            Assert.assertEquals( regionInfo.getMetaDomain(), null );
            Assert.assertEquals( regionInfo.getDataDomain(), null );
            Assert.assertEquals( regionInfo.getMetaLocation(), null );
            Assert.assertEquals( regionInfo.getMetaHisLocation(), null );
            Assert.assertEquals( regionInfo.getDataLocation(), null );
            regionClientNew.shutdown();
        }
    }

}

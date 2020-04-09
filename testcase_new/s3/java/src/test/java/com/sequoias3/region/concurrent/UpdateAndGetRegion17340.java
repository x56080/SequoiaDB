package com.sequoias3.region.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

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
    private String oldShardingType = "year";
    private String newShardingType = "month";
    private int fileSize = 1024 * 20;
    private File localPath = null;
    private String filePath = null;

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
        RegionUtils.clearRegion( regionName );
        Region region = new Region();
        region.withDataCSShardingType( oldShardingType )
                .withDataCLShardingType( oldShardingType )
                .withName( regionName );
        RegionUtils.putRegion( region );
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
                RegionUtils.deleteRegion( regionName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    @SuppressWarnings("deprecation")
    private void checkResult() throws Exception {
        Assert.assertTrue( RegionUtils.headRegion( regionName ) );

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
            Region region = new Region();
            region.withDataCSShardingType( newShardingType )
                    .withDataCLShardingType( newShardingType )
                    .withName( regionName );
            RegionUtils.putRegion( region );
        }
    }

    private class GetRegion extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            GetRegionResult result = RegionUtils.getRegion( regionName );
            Region regionInfo = result.getRegion();
            String dataCLShardingType = regionInfo.getDataCLShardingType();
            if ( dataCLShardingType.equals( newShardingType ) ) {
                Assert.assertEquals( regionInfo.getDataCSShardingType(),
                        newShardingType );
            } else {
                Assert.assertEquals( regionInfo.getDataCSShardingType(),
                        oldShardingType );
            }
            // get the region infor to take the default value
            Assert.assertEquals( regionInfo.getMetaDomain(), "" );
            Assert.assertEquals( regionInfo.getDataDomain(), "" );
            Assert.assertEquals( regionInfo.getMetaLocation(), "" );
            Assert.assertEquals( regionInfo.getMetaHisLocation(), "" );
            Assert.assertEquals( regionInfo.getDataLocation(), "" );
        }
    }

}

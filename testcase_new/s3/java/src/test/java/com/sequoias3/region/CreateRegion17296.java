package com.sequoias3.region;

import java.io.File;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17296: create Region and no specify config.
 * @author wuyan
 * @Date 2019.1.22
 * @version 1.00
 */
public class CreateRegion17296 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket17296";
    private String key = "key17296";
    private String regionName = "region17296";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 2;
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
    }

    @Test
    public void testRegion() throws Exception {
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );

        // get region and check region info
        checkRegion();

        // create object on region
        createObjectAndCheckResult();
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

    private void checkRegion() throws Exception {
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region regionInfo = result.getRegion();
        // get the region infor to take the default value.
        Assert.assertEquals( regionInfo.getDataCLShardingType(), "quarter" );
        Assert.assertEquals( regionInfo.getDataCSShardingType(), "year" );
        Assert.assertEquals( regionInfo.getMetaDomain(), "" );
        Assert.assertEquals( regionInfo.getDataDomain(), "" );
        Assert.assertEquals( regionInfo.getMetaLocation(), "" );
        Assert.assertEquals( regionInfo.getMetaHisLocation(), "" );
        Assert.assertEquals( regionInfo.getDataLocation(), "" );
    }

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult() throws Exception {
        s3Client.createBucket( bucketName, regionName );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

}

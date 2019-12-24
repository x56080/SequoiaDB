package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-16677: there are multiple versions of objects, head object
 *              without version
 * @author wuyan
 * @Date 2018.12.17
 * @version 1.00
 */
public class HeadObject16677 extends S3TestBase {
    private boolean runSuccess = false;
    private String key = "/test/object16677";
    private int fileSize = 1024 * 100;
    private int updateSize = 1024 * 200;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath1 =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        filePath2 =
                localPath + File.separator + "localFile_" + updateSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, fileSize );
        TestTools.LocalFile.createFile( filePath2, updateSize );

        s3Client = CommLib.buildS3Client();
        ObjectUtils.deleteObjectAllVersions( s3Client,
                S3TestBase.enableVerBucketName, key );
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                new File( filePath1 ) );
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                new File( filePath2 ) );
    }

    @Test
    public void testHeadObject() throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                S3TestBase.enableVerBucketName, key );
        ObjectMetadata result = s3Client.getObjectMetadata( request );
        Assert.assertEquals( result.getContentLength(), updateSize );
        Assert.assertEquals( result.getETag(), TestTools.getMD5( filePath2 ) );
        Assert.assertEquals( result.getVersionId(), "1" );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client,
                        S3TestBase.enableVerBucketName, key );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}

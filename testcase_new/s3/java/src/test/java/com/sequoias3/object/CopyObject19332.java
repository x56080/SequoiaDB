package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
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
 * @Description seqDB-19332: 指定ifMatch和ifNoneMatch条件匹配源对象复制
 * @author wuyan
 * @Date 2019.09.19
 * @version 1.00
 */
public class CopyObject19332 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19332";
    private String srcKeyName = "/src/bb%/object19332";
    private String destKeyName = "/dest/object19332";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 2;
    private File localPath = null;
    private String filePath = null;
    private String hisVersionContent = "testContent";

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, srcKeyName, hisVersionContent );
        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );
    }

    @Test
    public void testCopyObject() throws Exception {
        String curVersionEtag = TestTools.getMD5( filePath );
        String hisVersionETag = TestTools
                .getMD5( hisVersionContent.getBytes() );
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, destKeyName );
        request.withMatchingETagConstraint( curVersionEtag )
                .withNonmatchingETagConstraint( hisVersionETag );
        s3Client.copyObject( request );

        // check the content of destObject
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, destKeyName );
        Assert.assertEquals( downfileMd5, curVersionEtag );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}

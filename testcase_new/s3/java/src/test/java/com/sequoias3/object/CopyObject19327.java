package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.CopyObjectResult;
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
 * @Description seqDB-19327:复制对象指定if-Match条件
 * @author wuyan
 * @Date 2019.09.19
 * @version 1.00
 */
public class CopyObject19327 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19327";
    private String srcKeyName = "/src/bb%/object19327";
    private String destKeyName = "/dest/object19327";
    private String otherKeyName = "object19327";

    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 5;
    private File localPath = null;
    private String filePath = null;
    private String otherKeyContent = "testContent";

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );
        s3Client.putObject( bucketName, otherKeyName, otherKeyContent );
    }

    @Test
    public void testCopyObject() throws Exception {
        // test b: set Etag no match the etag of srcObject
        copyObjectWithNoMatchEtag();
        // test a: set Etag matching the etag of srcObject
        copyObjectWithMatchEtag();

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

    private void copyObjectWithMatchEtag() throws Exception {
        String eTag = TestTools.getMD5( filePath );
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, destKeyName );
        request.withMatchingETagConstraint( eTag );
        s3Client.copyObject( request );

        // check the content of get destObject
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, destKeyName );
        Assert.assertEquals( downfileMd5, eTag );
    }

    private void copyObjectWithNoMatchEtag() {
        String eTagB = TestTools.getMD5( otherKeyContent.getBytes() );
        CopyObjectRequest requestB = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, destKeyName );
        requestB.withMatchingETagConstraint( eTagB );
        CopyObjectResult result = s3Client.copyObject( requestB );
        Assert.assertNull( result, "does not match object!" );
    }
}

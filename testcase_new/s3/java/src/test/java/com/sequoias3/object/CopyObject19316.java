package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
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
 * @Description seqDB-19316:桶内复制对象
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19316 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName;
    private String srcKeyName = "srcObj19316";
    private String dstKeyName = "trgObj19316";
    private int fileSize = 6 * 1024 * 1024;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        bucketName = S3TestBase.bucketName;
        s3Client = CommLib.buildS3Client();
        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );
    }

    @Test
    private void test() throws Exception {
        s3Client.copyObject( bucketName, srcKeyName, bucketName, dstKeyName );
        checkObjectContent( bucketName, srcKeyName );
        checkObjectContent( bucketName, dstKeyName );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( bucketName, srcKeyName );
                s3Client.deleteObject( bucketName, dstKeyName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String bucketName, String keyName )
            throws Exception {
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }
}

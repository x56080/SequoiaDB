package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18103: bucket status is Suspended,create objects of the
 *              same name.
 * @author wuyan
 * @Date 2019.4.11
 * @version 1.00
 */
public class CreateObject18103 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "aa同名/bb/object18103";
    private String defaultDelimiter = "/";
    private String bucketName = "bucket18103";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 10;
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
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
    }

    @Test
    public void test() throws Exception {
        s3Client.putObject( bucketName, keyName, "firstContext18103" );
        s3Client.putObject( bucketName, keyName, new File( filePath ) );

        checkCreateObjectResult( bucketName );
        List<String> expContentList = new ArrayList<>();
        List<String> expCommprefixList = new ArrayList<>();
        expCommprefixList.add( "aa同名/" );
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                defaultDelimiter, expCommprefixList, expContentList );
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

    private void checkCreateObjectResult( String bucketName ) throws Exception {
        // check the content of the create object
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }
}

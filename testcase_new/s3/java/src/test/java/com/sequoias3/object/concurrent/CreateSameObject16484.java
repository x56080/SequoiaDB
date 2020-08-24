package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * @Description: 并发增加相同对象 testlink-case: seqDB-16484
 *
 * @author wangkexin
 * @Date 2018.12.18
 * @version 1.00
 */
public class CreateSameObject16484 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16484";
    private String bucketName = "bucket16484";
    private String keyName = "key16484";
    private String roleName = "normal";
    private int fileSize = 1024 * 1024 * 4;
    private File localPath = null;
    private String filePath = null;
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testCreateObject() throws Exception {
        CreateObjectThread createSameObject = new CreateObjectThread();
        createSameObject.start( 100 );

        Assert.assertTrue( createSameObject.isSuccess(),
                createSameObject.getErrorMsg() );

        checkCreateObjectResult( s3Client );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkCreateObjectResult( AmazonS3 s3Client ) throws Exception {
        ListObjectsV2Result listObjectsV2Result = s3Client
                .listObjectsV2( bucketName );
        Assert.assertEquals( listObjectsV2Result.getObjectSummaries().size(),
                1 );
        Assert.assertEquals( listObjectsV2Result.getObjectSummaries().get( 0 )
                .getBucketName(), bucketName, "bucketName is wrong!" );
        Assert.assertEquals(
                listObjectsV2Result.getObjectSummaries().get( 0 ).getKey(),
                keyName, "keyName is wrong!" );
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private class CreateObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            try {
                s3Client.putObject( bucketName, keyName, new File( filePath ) );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}

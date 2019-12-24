package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
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
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 禁用版本控制，并发获取不同版本的同一对象 testlink-case: seqDB-16511
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class GetSameObjectWithoutVersion16511 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16511";
    private String bucketName = "bucket16511";
    private String keyName = "key16511";
    private String roleName = "normal";
    private int fileSizeV1 = 1024 * 1024 * 2;
    private int fileSizeV2 = 1024 * 3;
    private int fileSizeV3 = 1024 * 3;
    private File localPath = null;
    private String filePathV1 = null;
    private String filePathV2 = null;
    private String filePathV3 = null;
    private List<String> etagList = new ArrayList<>();
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePathV1 =
                localPath + File.separator + "localFile_" + fileSizeV1 + ".txt";
        filePathV2 =
                localPath + File.separator + "localFile_" + fileSizeV2 + ".txt";
        filePathV3 =
                localPath + File.separator + "localFile_" + fileSizeV3 + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePathV1, fileSizeV1 );
        TestTools.LocalFile.createFile( filePathV2, fileSizeV2 );
        TestTools.LocalFile.createFile( filePathV3, fileSizeV3 );

        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put three versions of the object
        s3Client.putObject( bucketName, keyName, new File( filePathV1 ) );
        String downfileMd5V1 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        etagList.add( downfileMd5V1 );

        s3Client.putObject( bucketName, keyName, new File( filePathV2 ) );
        String downfileMd5V2 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        etagList.add( downfileMd5V2 );

        s3Client.putObject( bucketName, keyName, new File( filePathV3 ) );
        String downfileMd5V3 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        etagList.add( downfileMd5V3 );

        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
    }

    @Test
    public void testGetObject() throws Exception {
        // Getting different versions of objects
        List<GetDifferentObjectThread> getDiffVerObjects = new ArrayList<>();
        for ( int i = 0; i < 3; i++ ) {
            getDiffVerObjects
                    .add( new GetDifferentObjectThread( String.valueOf( i ) ) );
        }

        for ( GetDifferentObjectThread getDiffVerObject : getDiffVerObjects ) {
            getDiffVerObject.start();
        }

        for ( GetDifferentObjectThread getDiffVerObject : getDiffVerObjects ) {
            Assert.assertTrue( getDiffVerObject.isSuccess(),
                    getDiffVerObject.getErrorMsg() );
        }

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

    private class GetDifferentObjectThread extends S3ThreadBase {
        String versionid;

        public GetDifferentObjectThread( String versionid ) {
            this.versionid = versionid;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                S3Object object = s3Client.getObject(
                        new GetObjectRequest( bucketName, keyName,
                                versionid ) );
                S3ObjectInputStream s3is = object.getObjectContent();
                String downloadPath = TestTools.LocalFile
                        .initDownloadPath( localPath, TestTools.getMethodName(),
                                Thread.currentThread().getId() );
                ObjectUtils.inputStream2File( s3is, downloadPath );
                s3is.close();
                String getObjectMd5 = TestTools.getMD5( downloadPath );
                Assert.assertEquals( getObjectMd5,
                        etagList.get( Integer.parseInt( versionid ) ),
                        "md5 is wrong!" );
                Assert.assertEquals( object.getObjectMetadata().getVersionId(),
                        versionid );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}

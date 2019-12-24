package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * test content: 开启版本控制增加对象，桶中增加/删除多个对象 testlink-case: seqDB-16344
 *
 * @author wangkexin
 * @Date 2018.11.12
 * @version 1.00
 */
public class CreateObject16344 extends S3TestBase {
    String bucketName = "bucket16344";
    String keyName = "object16344";
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private Random random = new Random();
    private LinkedBlockingQueue<SaveEtagAndMd5> etag2md5 = new LinkedBlockingQueue<SaveEtagAndMd5>();
    private File localPath = null;
    private int defaultNums = 10;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testPutObject() throws Exception {
        List<PutObjectThread> putObjects = new ArrayList<>();

        for ( int i = 0; i < defaultNums; i++ ) {
            String subKeyName = keyName + "." + i;
            putObjects.add( new PutObjectThread( subKeyName ) );
        }

        for ( PutObjectThread putObject : putObjects ) {
            putObject.start();
        }

        for ( PutObjectThread putObject : putObjects ) {
            Assert.assertTrue( putObject.isSuccess(), putObject.getErrorMsg() );
        }

        checkCurrentObjectResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
            TestTools.LocalFile.removeFile( localPath );
        }
    }

    private void checkCurrentObjectResult() throws Exception {
        Assert.assertEquals( etag2md5.size(), 10000,
                "etag2md5 size is wrong! " );
        for ( int i = 0; i < 10000; i++ ) {
            SaveEtagAndMd5 expEtagAndMd5 = etag2md5.poll();
            GetObjectRequest request = new GetObjectRequest( bucketName,
                    expEtagAndMd5.getKeyName() );
            S3Object object = s3Client.getObject( request );
            String actEtag = object.getObjectMetadata().getETag();
            S3ObjectInputStream s3is = object.getObjectContent();
            String downloadPath = TestTools.LocalFile
                    .initDownloadPath( localPath, TestTools.getMethodName(),
                            expEtagAndMd5.getKeyName() );
            ObjectUtils.inputStream2File( s3is, downloadPath );
            s3is.close();
            String actMd5 = TestTools.getMD5( downloadPath );

            Assert.assertEquals( actEtag, expEtagAndMd5.getEtag(),
                    "etag is wrong , key name is:" + expEtagAndMd5
                            .getKeyName() );
            Assert.assertEquals( actMd5, expEtagAndMd5.getMd5(),
                    "MD5 is wrong , key name is:" + expEtagAndMd5
                            .getKeyName() );
        }
    }

    private class PutObjectThread extends S3ThreadBase {
        String keyName = null;

        public PutObjectThread( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            PutObjectResult putObjResult = null;
            try {
                for ( int i = 0; i < 1000; i++ ) {
                    int writeSize = random.nextInt( 1024 );
                    String currContent = ObjectUtils
                            .getRandomString( writeSize );
                    String currmd5 = TestTools.getMD5( currContent.getBytes() );
                    String currentName = keyName + "." + i;
                    putObjResult = s3Client
                            .putObject( bucketName, currentName, currContent );
                    etag2md5.offer( new SaveEtagAndMd5( currentName,
                            putObjResult.getETag(), currmd5 ) );
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class SaveEtagAndMd5 {
        private String keyName;
        private String etag;
        private String md5;

        public SaveEtagAndMd5( String keyName, String etag, String md5 ) {
            this.keyName = keyName;
            this.etag = etag;
            this.md5 = md5;
        }

        public String getKeyName() {
            return keyName;
        }

        public String getEtag() {
            return etag;
        }

        public String getMd5() {
            return md5;
        }
    }
}
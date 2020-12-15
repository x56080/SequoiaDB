package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
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
import java.util.HashMap;
import java.util.Map;

/**
 * @Description seqDB-19348:携带不同元数据并发复制对象，指定源和目标对象相同
 * @author wuyan
 * @Date 2019.09.20
 * @version 1.00
 */
public class CopyObject19348 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/bb%object19348";
    private String bucketName = "bucket19348";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 10;
    private File localPath = null;
    private String filePath = null;

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
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
    }

    @Test
    public void testCopyObject() throws Exception {
        // meta set the userMeta and contentDisposition
        Map< String, String > userMetaA = new HashMap<>();
        userMetaA.put( "tag1", "testa" );
        userMetaA.put( "tag2", "testa2" );
        String contentDispositionA = "this is copy objectA!";
        Map< String, String > userMetaB = new HashMap<>();
        userMetaB.put( "tag1", "testaB" );
        userMetaB.put( "tag2", "testab2" );
        userMetaB.put( "test3", "testab3" );
        String contentDispositionB = "this is copy objectB!";

        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec
                .addWorker( new CopyObject( userMetaA, contentDispositionA ) );
        threadExec
                .addWorker( new CopyObject( userMetaB, contentDispositionB ) );
        threadExec.run();

        checkObjectMetaData( userMetaA, userMetaB, contentDispositionA,
                contentDispositionB );
        checkObjectContent( bucketName, keyName );
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

    private void checkObjectContent( String bucketName, String keyName )
            throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectMetaData( Map< String, String > expMetaA,
            Map< String, String > expMetaB, String contentDispositionA,
            String contentDispositionB ) {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );

        Map< String, String > actMeta = result.getUserMetadata();
        if ( actMeta.size() == expMetaA.size() ) {
            for ( Map.Entry< String, String > entry : expMetaA.entrySet() ) {
                Object key = entry.getKey();
                Assert.assertEquals( actMeta.get( key ), expMetaA.get( key ),
                        "actMetaA = " + actMeta.toString() + ",expMetaA = "
                                + expMetaA.toString() );
            }
            Assert.assertEquals( result.getContentDisposition(),
                    contentDispositionA );
        } else {
            Assert.assertEquals( actMeta.size(), expMetaB.size(),
                    "expMetaB is : " + expMetaB.toString() + "actMetaB is : "
                            + actMeta.toString() );
            for ( Map.Entry< String, String > entry : expMetaB.entrySet() ) {
                Object key = entry.getKey();
                Assert.assertEquals( actMeta.get( key ), expMetaB.get( key ),
                        "actMetaB = " + actMeta.toString() + ",expMeta = "
                                + expMetaB.toString() );
            }
            Assert.assertEquals( result.getContentDisposition(),
                    contentDispositionB );
        }

    }

    private class CopyObject {
        private Map< String, String > userMeta = new HashMap<>();
        private String contentDisposition;
        private AmazonS3 s3Client1 = CommLib.buildS3Client();
        private ObjectMetadata metaData = new ObjectMetadata();

        private CopyObject( Map< String, String > userMeta,
                String contentDisposition ) {
            this.userMeta = userMeta;
            this.contentDisposition = contentDisposition;
        }

        @ExecuteOrder(step = 1)
        private void setUserMetadata() {
            metaData = new ObjectMetadata();
            metaData.setUserMetadata( userMeta );
            metaData.setContentDisposition( contentDisposition );
        }

        @ExecuteOrder(step = 2)
        private void copyObject() throws Exception {
            try {
                CopyObjectRequest request = new CopyObjectRequest( bucketName,
                        keyName, bucketName, keyName );
                request.setMetadataDirective( "REPLACE" );
                request.withNewObjectMetadata( metaData );
                s3Client1.copyObject( request );
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }
}

package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-16494:concurrent create and get the same object
 * @author wuyan
 * @Date 2019.1.8
 * @version 1.00
 */
public class CreateAndListObject16494 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16494";
    private String keyName = "aa%bb%object16494";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private int objectNums = 10;
    private List< String > keyList = new ArrayList<>();
    private List< String > queryKeyList1 = new ArrayList<>();
    private List< String > queryKeyList2 = new ArrayList<>();

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
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testCreateBucket() throws Exception {
        List< PutObjectThread > putObjectThreads = new ArrayList<>(
                objectNums );
        ListObjectThread listObjectThread = new ListObjectThread();
        ListObjectV1Thread listObjectV1Thread = new ListObjectV1Thread();
        for ( int i = 0; i < objectNums; i++ ) {
            String key = keyName + "_" + i;
            keyList.add( key );
            putObjectThreads.add( new PutObjectThread( key ) );
        }
        for ( PutObjectThread putObjectThread : putObjectThreads ) {
            putObjectThread.start();
        }

        listObjectThread.start( 10 );
        listObjectV1Thread.start( 10 );

        for ( PutObjectThread putObjectThread : putObjectThreads ) {
            Assert.assertTrue( putObjectThread.isSuccess(),
                    putObjectThread.getErrorMsg() );
        }
        Assert.assertTrue( listObjectThread.isSuccess(),
                listObjectThread.getErrorMsg() );
        Assert.assertTrue( listObjectV1Thread.isSuccess(),
                listObjectV1Thread.getErrorMsg() );

        // check the query keys by listObjectv1 and listObjectv2, than check the
        // all keys
        listObjectResult( queryKeyList1 );
        listObjectResult( queryKeyList2 );
        listObjectsAndCheckResult( keyList );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void listObjectsAndCheckResult( List< String > keyList )
            throws IOException {
        List< String > queryKeyList = new ArrayList<>();
        ListObjectsV2Result result = s3Client.listObjectsV2( bucketName );
        List< S3ObjectSummary > objects = result.getObjectSummaries();
        Assert.assertEquals( objects.size(), objectNums );
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            queryKeyList.add( key );
        }

        // check the keyName
        Collections.sort( keyList );
        // TODO :queryKeyList不必排序
        Collections.sort( queryKeyList );
        Assert.assertEquals( queryKeyList, keyList );
    }

    private void listObjectResult( List< String > queryKeyList ) {
        for ( String key : queryKeyList ) {
            if ( !keyList.contains( key ) ) {
                Assert.fail( "list key error!,the key is " + key
                        + "\nqueryList:" + queryKeyList.toString() );
            }
        }

    }

    private class PutObjectThread extends S3ThreadBase {
        private String keyName;

        public PutObjectThread( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                s3Client.putObject( bucketName, keyName, new File( filePath ) );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                ListObjectsV2Result result = s3Client
                        .listObjectsV2( bucketName );
                List< S3ObjectSummary > objects = result.getObjectSummaries();
                for ( S3ObjectSummary os : objects ) {
                    String key = os.getKey();
                    queryKeyList2.add( key );
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListObjectV1Thread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {

                ObjectListing result = s3Client.listObjects( bucketName );
                List< S3ObjectSummary > objects = result.getObjectSummaries();
                for ( S3ObjectSummary os : objects ) {
                    String key = os.getKey();
                    queryKeyList1.add( key );
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}

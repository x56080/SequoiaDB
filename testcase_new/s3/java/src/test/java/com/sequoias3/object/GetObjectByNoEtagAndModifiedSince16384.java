package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;

/**
 * @Description: seqDB-16384 ::
 *               指定ifNoneMatch和ifModifiedSince条件获取对象，不匹配ifNoneMatch
 * @author fanyu
 * @Date:2018年11月14日
 * @version:1.0
 */

public class GetObjectByNoEtagAndModifiedSince16384 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16384";
    private String objectName = "object16384";
    private AmazonS3 s3Client = null;
    private int fileSize = 3;
    private File localPath = null;
    private List<String> filePathList = new ArrayList<String>();
    private List<PutObjectResult> objectVSList = new ArrayList<PutObjectResult>();
    private int fileNum = 5;
    private Calendar cal = Calendar.getInstance();

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        String filePath = null;
        for ( int i = 0; i < fileNum; i++ ) {
            filePath =
                    localPath + File.separator + "localFile_" + ( fileSize + i )
                            + ".txt";
            TestTools.LocalFile.createFile( filePath, fileSize + i );
            filePathList.add( filePath );
        }
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void test() throws Exception {
        // create multiple versions object in the bucket
        for ( int i = 0; i < fileNum; i++ ) {
            objectVSList.add( s3Client.putObject(
                    new PutObjectRequest( bucketName, objectName,
                            new File( filePathList.get( i ) ) ) ) );
        }

        // get current eTag
        int currIndex = fileNum - 1;
        String currETag = objectVSList.get( currIndex ).getETag();

        // get object by NonMatchingETag and modified
        // the object has been modified since now-one_month
        cal.set( Calendar.DAY_OF_MONTH, cal.get( Calendar.DAY_OF_MONTH ) - 1 );
        S3Object currObject = s3Client.getObject(
                new GetObjectRequest( bucketName, objectName )
                        .withNonmatchingETagConstraint( currETag )
                        .withModifiedSinceConstraint( cal.getTime() ) );
        Assert.assertNull( currObject );
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
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}

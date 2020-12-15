package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: 空桶删除对象
 *
 * @author wangkexin
 * @Date 2018.11.28
 * @version 1.00
 */

public class DeleteObject16457 extends S3TestBase {
    private String bucketName = "bucket16457";
    private String keyName = "testkey16457";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // delete object in empty bucket
        s3Client.deleteObject( bucketName, keyName );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            s3Client.deleteBucket( bucketName );
        }
    }
}

package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18111: To get a list of objects within a bucket.exist the
 *              delete tag,the object name include delimiter.
 * @author wuyan
 * @Date 2019.04.15
 * @version 1.00
 */
public class ListObjects18111 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18111";
    private String keyName = "aa%bb%object18111";
    private String defaultDelimiter = "/";
    private AmazonS3 s3Client = null;
    private int objectNums = 100;
    private List<String> matchKeyList = new ArrayList<>();

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testCreateObject() throws Exception {
        for ( int i = 0; i < objectNums / 2; i++ ) {
            String subKeyName = keyName + "_" + i + "/aa/test.png";
            s3Client.putObject( bucketName, subKeyName,
                    "testcontext18111_" + i );
            matchKeyList.add( keyName + "_" + i + "/" );

            // create object with delete tag
            String delteKeyName = keyName + "_deletetag_" + i + "/aa/test.png";
            s3Client.deleteObject( bucketName, delteKeyName );
            matchKeyList.add( keyName + "_deletetag_" + i + "/" );
        }

        List<String> expContentList = new ArrayList<>();
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                defaultDelimiter, matchKeyList, expContentList );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}

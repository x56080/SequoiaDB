package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18079: the object name include old delimiter, than update
 *              the old delimiter to the new delimiter.
 * @author wuyan
 * @Date 2019.04.09
 * @version 1.00
 */

public class UpdateDelimiter18079 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18079";
    private String keyName = "/test/aa/object18079";
    private int keyNums = 10;
    private String delimiter = "%";
    private List<String> expContentList = new ArrayList<>();
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        for ( int i = 0; i < keyNums; i++ ) {
            String subKeyName = keyName + "_" + i + "_test.png";
            s3Client.putObject( bucketName, subKeyName, "context18079" + i );
            expContentList.add( subKeyName );
        }
    }

    @Test
    public void testUpdateDelimiter() throws Exception {
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        List<String> expCommprefixList = new ArrayList<>();
        DelimiterUtils
                .listObjectsWithDelimiter( s3Client, bucketName, delimiter,
                        expCommprefixList, expContentList );
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

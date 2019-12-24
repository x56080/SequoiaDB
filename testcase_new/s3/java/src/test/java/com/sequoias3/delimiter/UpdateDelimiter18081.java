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
 * @Description seqDB-18081: current delimiter is delimiter2, delimiter1 is
 *              exists, than update the delimiter2 to the new delimiter.
 * @author wuyan
 * @Date 2019.04.10
 * @version 1.00
 */
public class UpdateDelimiter18081 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18081";
    private String keyName = "aa?%bb?cc?%test1_18081.png";
    private String newDelimiter = "c";
    private String curDelimiter = "cc";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, "updatedelimite18081" );
    }

    @Test
    public void testUpdateDelimiter() throws Exception {
        DelimiterUtils.putBucketDelimiter( bucketName, curDelimiter );
        DelimiterUtils.updateDelimiterSuccessAgain( bucketName, newDelimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, newDelimiter );

        List<String> expCommprefixList = new ArrayList<>();
        expCommprefixList.add( "aa?%bb?c" );
        List<String> expContentList = new ArrayList<>();
        DelimiterUtils
                .listObjectsWithDelimiter( s3Client, bucketName, newDelimiter,
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

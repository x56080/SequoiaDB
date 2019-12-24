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
 * @Description seqDB-18080: update delimiter, the new delimiter is the same as
 *              the new delimiter.
 * @author wuyan
 * @Date 2019.04.09
 * @version 1.00
 */
public class UpdateDelimiter18080 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18080";
    private String keyName = "aa%test/maa%/object18080";
    private String delimiter = "/";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );

        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, "testdelimiter18080" );
    }

    @Test
    public void testUpdateDelimiter() throws Exception {
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );
        List<String> expCommprefixList = new ArrayList<>();
        expCommprefixList.add( "aa%test/" );
        List<String> expContentList = new ArrayList<>();
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

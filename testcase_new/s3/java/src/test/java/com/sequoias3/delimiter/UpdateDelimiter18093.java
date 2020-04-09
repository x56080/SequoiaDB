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
 * @Description seqDB-18093: get default delimiter.
 * @author wuyan
 * @Date 2019.04.10
 * @version 1.00
 */
public class UpdateDelimiter18093 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18093";
    private String keyName = "/test/aa/object18093";
    private String delimiter = "/";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );

        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, "context18093" );
    }

    @Test
    public void testUpdateDelimiter() throws Exception {
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );
        List< String > expCommprefixList = new ArrayList<>();
        expCommprefixList.add( "/" );
        List< String > expContentList = new ArrayList<>();
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                delimiter, expCommprefixList, expContentList );
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

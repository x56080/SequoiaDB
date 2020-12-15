package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description: 设置分隔符，指定分隔符为多字符 testlink-case: seqDB-18090
 *
 * @author wangkexin
 * @Date 2019.04.12
 * @version 1.00
 */
public class UpdateDelimiter18090 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String bucketName = "bucket18090";
    private String userName = "user18090";
    private String roleName = "normal";
    private String[] objectNames = { "/test18090/a/1.txt",
            "/test18090//2aa.txt", "/testaaa18090//3.txt" };
    private AmazonS3 s3Client = null;
    private String[] accessKeys = null;

    @DataProvider(name = "delimiterProvider")
    public Object[][] delimiterProvider() {
        // 对象名覆盖单字符和多字符
        return new Object[][] { { "a" }, { "aa" }, { "aaa" }, };
    }

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test(dataProvider = "delimiterProvider")
    private void testUpdateDelimiter( String newDelimiter ) throws Exception {
        s3Client.createBucket( bucketName );
        for ( int i = 0; i < objectNames.length; i++ ) {
            s3Client.putObject( bucketName, objectNames[ i ], "test18084" );
        }

        DelimiterUtils.putBucketDelimiter( bucketName, newDelimiter,
                accessKeys[ 0 ] );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, newDelimiter,
                accessKeys[ 0 ] );

        List< String > expCommonPrefixes = ObjectUtils
                .getCommPrefixes( objectNames, "", newDelimiter );
        List< String > matchContentsList = ObjectUtils.getKeys( objectNames, "",
                newDelimiter );
        DelimiterUtils.listObjectsWithDelimiter( s3Client, bucketName,
                newDelimiter, expCommonPrefixes, matchContentsList );

        CommLib.clearBucket( s3Client, bucketName );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == delimiterProvider().length ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}

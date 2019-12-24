package com.sequoias3.config;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.web.client.HttpStatusCodeException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

/**
 * test content: 开启鉴权，authorization头部非法格式校验testlink-case: seqDB-18591
 *
 * @author wangkexin
 * @Date 2019.06.24
 * @version 1.00
 */
public class CreateBucket18591 extends S3TestBase {
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );

    @DataProvider(name = "authorizationProvider", parallel = true)
    public Object[][] generateAuthorization() {
        return new Object[][] {
                // test a : authorization 头部不存在
                new Object[] { "" },
                // test b : authorization头部存在“Credential=”字符串，但字符串后面不存在“/”
                new Object[] {
                        UserCommDefind.authValPre + UserUtils.accessKeyId },
                // test c : authorization头部为version2版本，但“：”后无signature
                new Object[] { "AWS " + UserUtils.accessKeyId + ":" },
                // test d : authorization头部不属于v2,v4任意一种格式
                new Object[] { "AWS " + UserCommDefind.authValPre
                        + UserUtils.accessKeyId + ":test" },
                // test e : 符合v2格式，但是AccessKeyId里面包含“Credential=”
                new Object[] { "AWS " + UserCommDefind.authValPre
                        + UserUtils.accessKeyId + ":signature" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
    }

    @Test(dataProvider = "authorizationProvider")
    private void testCreateBucket( String authorization ) throws Exception {
        // create bucket
        try {
            createBucket( bucketName, authorization );
            Assert.fail( "expect failed but found succeed" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(),
                    "AuthorizationHeaderMalformed" );
        }
    }

    @AfterClass
    private void tearDown() throws Exception {
    }

    private void createBucket( String bucketName, String authorization )
            throws UnsupportedEncodingException {
        TestRest rest = new TestRest( type );
        try {
            rest.setApi( URLEncoder.encode( bucketName, "UTF-8" ) )
                    .setRequestMethod( HttpMethod.PUT )
                    .setRequestHeaders( UserCommDefind.authorization,
                            authorization ).setResponseType( String.class )
                    .exec();
        } catch ( HttpStatusCodeException e ) {
            throw DelimiterUtils.httpToAmazon( e );
        }
    }
}

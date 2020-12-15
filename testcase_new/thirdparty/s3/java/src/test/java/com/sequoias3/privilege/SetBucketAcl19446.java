package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Permission;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-19446: 同时使用两种或两种以上的方式配置桶acl
 * @Author wangkexin
 * @Date 2019.09.20
 */
public class SetBucketAcl19446 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private int expRunSuccessNum = 4;
    private String bucketName = "bucket19446";
    private AmazonS3 s3Client = null;
    private String ownerId;
    private String displayName;
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );

    @DataProvider(name = "methodProvider")
    private Object[][] generateMethodProvider() {
        // parameter : setByStandardAcl, setByHeader, setByBody
        return new Object[][] {
                // 同时使用标准acl和x-amz-grant-*方式配置桶acl
                new Object[] { true, true, false },
                // 同时使用标准acl和body方式配置桶acl
                new Object[] { true, false, true },
                // 同时使用x-amz-grant-*方式和body方式配置桶acl
                new Object[] { false, true, true },
                // 同时使用标准acl和x-amz-grant-*方式以及body方式配置桶acl
                new Object[] { true, true, true }, };
    }

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        ownerId = s3Client.getS3AccountOwner().getId();
        displayName = s3Client.getS3AccountOwner().getDisplayName();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test(dataProvider = "methodProvider")
    private void testSetBucketAcl( boolean setByStandardAcl,
            boolean setByHeader, boolean setByBody ) throws Exception {
        TestRest rest = new TestRest( type );
        try {
            String body = "<AccessControlPolicy><Owner><ID>" + ownerId
                    + "</ID><DisplayName>" + displayName
                    + "</DisplayName></Owner><AccessControlList><Grant><Grantee type=\"CanonicalUser\"><ID>"
                    + ownerId + "</ID><DisplayName>" + displayName
                    + "</DisplayName></Grantee><Permission>FULL_CONTROL</Permission></Grant></AccessControlList></AccessControlPolicy>";
            rest.setApi( "/" + bucketName + "/?acl" )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + S3TestBase.s3AccessKeyId
                                    + "/" )
                    .setRequestMethod( HttpMethod.PUT )
                    .setResponseType( String.class );
            if ( setByStandardAcl ) {
                rest.setRequestHeaders( "x-amz-acl", "private" );
            }
            if ( setByHeader ) {
                rest.setRequestHeaders( "x-amz-grant-read", "id=" + ownerId );
            }
            if ( setByBody ) {
                rest.setRequestBody( body );
            }
            rest.exec();
            Assert.fail(
                    "setting bucket acl in two or more ways at the same time should fail." );
        } catch ( HttpClientErrorException e ) {
            AmazonS3Exception amazonS3Exception = RegionUtils.httpToAmazon( e );
            if ( !amazonS3Exception.getErrorCode()
                    .equals( "InvalidRequest" ) ) {
                throw amazonS3Exception;
            }
        }

        checkDefaultSettings();
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == expRunSuccessNum ) {
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkDefaultSettings() {
        Grant expGrant = new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl );
        PrivilegeUtils.checkSetBucketAclResult( s3Client, bucketName,
                expGrant );
    }
}

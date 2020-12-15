package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.EmailAddressGrantee;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Grantee;
import com.amazonaws.services.s3.model.GroupGrantee;
import com.amazonaws.services.s3.model.Permission;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-19444: 桶开启版本控制，配置桶acl，被授权人包含非owner的用户
 * @Author wangkexin
 * @Date 2019.09.20
 */
public class SetBucketAcl19444 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private int expRunSuccessNum = 13;
    private String keyName = "key19444";
    private String userName = "user19444";
    private String roleName = "normal";
    private String bucketName = "bucket19444";
    private String[] acessKeys = null;
    private AmazonS3 userS3Client = null;
    private AmazonS3 ownerS3Client = null;
    private long oldFileSize = 100 * 1024;
    private long newFileSize = 200 * 1024;
    private File localPath = null;
    private File oldFile = null;
    private File newFile = null;
    private String oldFilePath = null;
    private String newFilePath = null;
    private String ownerId;
    private String userId;

    @DataProvider(name = "aclProvider")
    private Object[][] generateAclProvider() {
        // parameter : CannedAccessControlList acl, Grant[] expGrant
        return new Object[][] {
                // set bucket acl : public-read
                new Object[] { CannedAccessControlList.PublicRead, new Grant[] {
                        new Grant( new CanonicalGrantee( ownerId ),
                                Permission.FullControl ),
                        new Grant( GroupGrantee.AllUsers, Permission.Read ) } },
                // set bucket acl : public-read-write
                new Object[] { CannedAccessControlList.PublicReadWrite,
                        new Grant[] {
                                new Grant( new CanonicalGrantee( ownerId ),
                                        Permission.FullControl ),
                                new Grant( GroupGrantee.AllUsers,
                                        Permission.Read ),
                                new Grant( GroupGrantee.AllUsers,
                                        Permission.Write ) } },
                // set bucket acl : authenticated-read
                new Object[] { CannedAccessControlList.AuthenticatedRead,
                        new Grant[] {
                                new Grant( new CanonicalGrantee( ownerId ),
                                        Permission.FullControl ),
                                new Grant( GroupGrantee.AuthenticatedUsers,
                                        Permission.Read ) } } };

    }

    @DataProvider(name = "grantProvider")
    private Object[][] generateGrantProvider() {
        // parameter : grantee
        return new Object[][] {
                // set bucket acl grantee: id of non-bucket owner
                new Object[] { new CanonicalGrantee( userId ) },
                // set bucket acl grantee: uri(a perdefined s3 group)
                new Object[] { GroupGrantee.AllUsers },
                new Object[] { GroupGrantee.AuthenticatedUsers },
                new Object[] { GroupGrantee.LogDelivery },
                // set bucket acl grantee: emailAddress
                new Object[] { new EmailAddressGrantee(
                        "test19444 email address" ) } };
    }

    @BeforeClass
    private void setUp() throws IOException {
        // create a user
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        userS3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        userId = userS3Client.getS3AccountOwner().getId();

        ownerS3Client = CommLib.buildS3Client();
        ownerId = ownerS3Client.getS3AccountOwner().getId();
        CommLib.clearBucket( ownerS3Client, bucketName );
        ownerS3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( ownerS3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        putObjectBeforeTest();

    }

    @Test(dataProvider = "aclProvider")
    private void testSetBucketAcl1( CannedAccessControlList acl,
            Grant[] expGrant ) throws Exception {
        // set bucket acl using standard acl mode
        ownerS3Client.setBucketAcl( bucketName, acl );
        PrivilegeUtils.checkSetBucketAclResult( ownerS3Client, bucketName,
                expGrant );
        getObjectByOtherUser();
        actSuccessTests.getAndIncrement();
    }

    @Test(dataProvider = "grantProvider")
    private void testSetBucketAcl2( Grantee grantee ) throws Exception {
        // set bucket acl with x-amz-grant-* in the request header
        for ( Permission permission : Permission.values() ) {
            Grant expGrant = new Grant( grantee, permission );
            PrivilegeUtils.setBucketAclByHeader( s3AccessKeyId, bucketName,
                    expGrant );
            PrivilegeUtils.checkSetBucketAclResult( ownerS3Client, bucketName,
                    expGrant );
            getObjectByOtherUser();
        }
        actSuccessTests.getAndIncrement();
    }

    @Test(dataProvider = "grantProvider")
    private void testSetBucketAcl3( Grantee grantee ) throws Exception {
        // set bucket acl with access control list in request body
        for ( Permission permission : Permission.values() ) {
            Grant expGrant = new Grant( grantee, permission );
            PrivilegeUtils.setBucketAclByBody( ownerS3Client, bucketName,
                    expGrant );
            PrivilegeUtils.checkSetBucketAclResult( ownerS3Client, bucketName,
                    expGrant );
            getObjectByOtherUser();
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == expRunSuccessNum ) {
                CommLib.clearBucket( ownerS3Client, bucketName );
                CommLib.clearUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            ownerS3Client.shutdown();
            userS3Client.shutdown();
        }
    }

    private void getObjectByOtherUser() throws Exception {
        String expMd5 = TestTools.getMD5( newFilePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( userS3Client,
                localPath, bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
    }

    private void putObjectBeforeTest() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        oldFilePath = localPath + File.separator + "localFile_" + oldFileSize
                + ".txt";
        newFilePath = localPath + File.separator + "localFile_" + newFileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( oldFilePath, oldFileSize );
        TestTools.LocalFile.createFile( newFilePath, newFileSize );
        oldFile = new File( oldFilePath );
        newFile = new File( newFilePath );

        ownerS3Client.putObject( bucketName, keyName, oldFile );
        ownerS3Client.putObject( bucketName, keyName, newFile );
    }
}

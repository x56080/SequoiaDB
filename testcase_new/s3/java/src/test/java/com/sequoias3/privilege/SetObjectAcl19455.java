package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
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
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-19455: 桶禁用版本控制，配置对象acl，被授权人包含多种用户
 * @Author wangkexin
 * @Date 2019.09.20
 */
public class SetObjectAcl19455 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private int expRunSuccessNum = 2;
    private String bucketName = "bucket19455";
    private String keyName = "key19455";
    private String userName = "user19455";
    private String roleName = "normal";
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

    @DataProvider(name = "methodProvider")
    private Object[][] generateMethodProvider() {
        // parameter : method
        return new Object[][] {
                // set object acl with x-amz-grant-* in the request header
                new Object[] { "setByRequestHeader" },
                // set object acl with access control list in request body
                new Object[] { "setByRequestBody" } };
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
                BucketVersioningConfiguration.SUSPENDED );
        putObjectBeforeTest();
    }

    @Test(dataProvider = "methodProvider")
    private void testSetObjectAcl( String methodOfSetBucketAcl )
            throws Exception {
        List< Grant > expGrantList = new ArrayList<>();
        // grantee include: id:ownerId , non-owner Id, perdefined group,
        // emailAddress
        Grantee[] grantees = { new CanonicalGrantee( ownerId ),
                new CanonicalGrantee( userId ), GroupGrantee.AllUsers,
                GroupGrantee.AuthenticatedUsers, GroupGrantee.LogDelivery,
                new EmailAddressGrantee(
                        "test19455 email address " + methodOfSetBucketAcl ) };
        for ( Grantee grantee : grantees ) {
            for ( Permission permission : Permission.values() ) {
                Grant grant = new Grant( grantee, permission );
                expGrantList.add( grant );
            }
        }
        Grant[] expGrant = expGrantList
                .toArray( new Grant[ expGrantList.size() ] );

        switch ( methodOfSetBucketAcl ) {
        case "setByRequestHeader":
            PrivilegeUtils.setObjectAclByHeader( s3AccessKeyId, bucketName,
                    keyName, expGrant );
            break;
        case "setByRequestBody":
            PrivilegeUtils.setObjectAclByBody( ownerS3Client, bucketName,
                    keyName, expGrant );
            break;
        }
        PrivilegeUtils.checkSetObjectAclResult( ownerS3Client, bucketName,
                keyName, expGrant );
        getObjectByOtherUser();
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

package com.sequoiadb.driveconnection.serial;

import com.sequoiadb.auth.Util;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.UserConfig;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * @descreption seqDB-27519:userConfig(UserConfig userConfig)设置用户配置
 *              seqDB-27526:userConfig() 创建用户配置对象 seqDB-27527:userConfig(String
 *              userName, String password) 创建用户配置对象
 *              seqDB-27528:userConfig(String userName, File cipherFile)
 *              创建用户配置对象 seqDB-27529:userConfig(String userName, File
 *              cipherFile, String token) 创建用户配置对象
 * @author Xu Mingxing
 * @date 2022/9/14
 * @updateUser
 * @updateDate
 * @updateRemark
 * @version 1.0
 */

public class Connection27519_27526_27527_27528_27529 extends SdbTestBase {
    private Sequoiadb db = null;
    private Sequoiadb sdb = null;
    private String csName = "cs_27519";
    private String userName = "user_27519";
    private String password = "password_27519";
    private String passwdFileName = "/password27519";
    private String passwordFilePath = null;
    private String token = "27519";

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
    }

    @Test
    public void test() throws Exception {
        // test a：不存在鉴权用户,不指定userConfig接口
        sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl ).build();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test b：不存在鉴权用户,指定userConfig接口
        // 默认创建
        UserConfig userConfig1 = new UserConfig();
        sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( userConfig1 ).build();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );

        // test c：存在鉴权用户,用户配置信息正确
        // 使用用户名、密码创建
        db.createUser( userName, password );
        UserConfig userConfig2 = new UserConfig( userName, password );
        sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( userConfig2 ).build();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );
        Assert.assertEquals( userConfig2.getUserName(), userName,
                "The expected result are equal" );
        Assert.assertEquals( userConfig2.getPassword(), password,
                "The expected result are equal" );
        Assert.assertEquals( userConfig2.toString(),
                "UserConfig{userName='" + userName + "'}",
                "The expected result are equal" );
        Assert.assertEquals( userConfig1.equals( userConfig2 ), false,
                "The expected result are equal" );
        Assert.assertNotEquals( userConfig1.hashCode(), userConfig2.hashCode(),
                "The expected result are not equal" );

        // 使用用户名、密码文件创建
        String toolsPath = Util.getSdbInstallDir() + "/bin/";
        Util.createPasswdFile( userName, password, passwdFileName );
        Util.downLoadFileToLocal( SdbTestBase.workDir,
                toolsPath + passwdFileName );
        passwordFilePath = SdbTestBase.workDir + passwdFileName;
        userConfig2 = new UserConfig( userName, new File( passwordFilePath ) );
        sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( userConfig2 ).build();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );
        Assert.assertEquals( userConfig2.getUserName(), userName,
                "The expected result are equal" );
        Assert.assertEquals( userConfig2.getCipherFile(),
                new File( passwordFilePath ), "The expected result are equal" );

        // 使用用户名、密码文件、token创建
        Util.createPasswdFile( userName, password, passwdFileName, token );
        Util.downLoadFileToLocal( SdbTestBase.workDir,
                toolsPath + passwdFileName );
        userConfig2 = new UserConfig( userName, new File( passwordFilePath ),
                token );
        sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl )
                .userConfig( userConfig2 ).build();
        sdb.createCollectionSpace( csName );
        sdb.dropCollectionSpace( csName );
        Assert.assertEquals( userConfig2.getUserName(), userName,
                "The expected result are equal" );
        Assert.assertEquals( userConfig2.getCipherFile(),
                new File( passwordFilePath ), "The expected result are equal" );
        Assert.assertEquals( userConfig2.getToken(), token,
                "The expected result are equal" );

        // test d：存在鉴权用户,用户配置信息不正确
        userConfig2 = new UserConfig( userName, "" );
        try {
            sdb = Sequoiadb.builder().serverAddress( SdbTestBase.coordUrl )
                    .userConfig( userConfig2 ).build();
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    @AfterClass
    public void tearDown() throws Exception {
        new File( passwordFilePath ).deleteOnExit();
        Util.removePasswdFile(
                Util.getSdbInstallDir() + "/bin" + passwdFileName );
        db.removeUser( userName, password );
        if ( db != null ) {
            db.close();
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}

package com.sequoiadb.auth;

import com.sequoiadb.exception.SDBError;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:TestSdbUser16280 Connecting sequoiadb with incorrect password
 * @FileName:seqDB-29772 通过evaljs 方法鉴权
 *
 * @author wangkexin
 * @Date 2018-10-22
 * @version 1.00
 */

public class TestSdbUser16280 extends SdbTestBase {
    private Sequoiadb sdb;
    private String coordAddr;
    private String userName = "admin16280";
    private String password = "admin";

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        sdb = new Sequoiadb( this.coordAddr, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "run mode is standalone,test case skip" );
        }
    }

    @Test
    public void test() {
        sdb.createUser( userName, password );
        try {
            Sequoiadb errorConn = new Sequoiadb( coordAddr, userName, "" );
            Assert.fail( "exp fail but act success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN.getErrorCode(), e.getErrorCode() );
        }

        String code = "var db = new Sdb(\'" + hostName + "\'," + serviceName + ",\'" + userName + "\', \'" + password + "\');";
        Sequoiadb.SptEvalResult result1 = sdb.evalJS( code );
        Assert.assertEquals( null, result1.getErrMsg() );

        String errCode = "var db = new Sdb(\'" + hostName + "\'," + serviceName + ",\'" + userName + "\', \'" + "" + "\');";
        Sequoiadb.SptEvalResult result2 = sdb.evalJS( errCode );
        if ( null == result2.getErrMsg() ) {
            Assert.fail("exp fail but act success" );
        } else {
            Assert.assertEquals( SDBError.SDB_AUTH_AUTHORITY_FORBIDDEN.getErrorCode(), result2.getErrMsg().get( "retCode" ) );
        }
    }

    @AfterClass(alwaysRun = true)
    public void tearDown() {
        sdb.removeUser( userName, password );
        sdb.close();
    }
}
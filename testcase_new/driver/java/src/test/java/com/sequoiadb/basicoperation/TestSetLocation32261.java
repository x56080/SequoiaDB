package com.sequoiadb.basicoperation;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @version 1.0
 * @Description seqDB-32261:Java驱动domain.setLocation接口参数校验
 * @Author TangTao
 * @Date 2023.06.21
 * @UpdateAuthor TangTao
 * @UpdateDate 2023.06.21
 */

public class TestSetLocation32261 extends SdbTestBase {
    private String domainName = "domain_32261";
    private static Sequoiadb sdb = null;
    private Domain domain;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isDomainExist( domainName ) ) {
            sdb.dropDomain( domainName );
        }
        domain = sdb.createDomain( domainName, new BasicBSONObject() );
    }

    @Test
    public void testLob() {
        StringBuilder sb = new StringBuilder( 300 );
        for ( int i = 0; i < 30; i++ ) {
            sb.append( "aaaaaaaaaa" );
        }
        String location1 = "location_32259_1";
        String location2 = sb.toString();
        String hostName = SdbTestBase.hostName;


        // case 1: setLocation接口，参数hostname 为空字符串
        domain.setLocation( "", location1 );

        // case 2: setLocation接口，参数hostname 为字符串类型，值为一个有效主机名
        domain.setLocation( hostName, location1 );

        // case 3: setLocation接口，参数location 为空字符串
        domain.setLocation( hostName, "" );

        // case 4: setLocation接口，参数location 为字符串类型，长度超过256个字符
        try {
            domain.setLocation( hostName, location2 );
            Assert.fail( "location length was too long, Expected to failed." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_INVALIDARG.getErrorCode() )
                throw ( e );
        }
    }

    @AfterClass
    public void tearDown() {
        domain.setLocation( hostName, "" );
        if ( sdb.isDomainExist( domainName ) ) {
            sdb.dropDomain( domainName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }
}

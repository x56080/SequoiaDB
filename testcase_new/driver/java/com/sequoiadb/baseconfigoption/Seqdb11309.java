package com.sequoiadb.baseconfigoption;

import com.sequoiadb.net.ConfigOptions;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.Date;

import static org.testng.Assert.*;

/**
 * 覆盖的测试用例：seqDB-11309,seqDB-11310,seqDB-11311,seqDB-11312,seqDB-11313,
 * 测试点：ConfigOptions的set、get方法，以及默认值 Created by laojingtang on 17-4-10.
 */
public class Seqdb11309 extends SdbTestBase {

    @BeforeClass
    public void setUp() {
        System.out.println( "the TestCase Name:" + this.getClass().getName()
                + ". the TestCase begin at:"
                + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss.SSS" )
                        .format( new Date() ) );
    }

    @Test
    public void test() {
        ConfigOptions options = new ConfigOptions();
        // 默认值测试
        assertTrue( false == options.getUseNagle() );
        assertTrue( 0 == options.getSocketTimeout() );
        assertTrue( 1000 * 10 == options.getConnectTimeout() );
        assertTrue( 15000 == options.getMaxAutoConnectRetryTime() );
        assertTrue( true == options.getSocketKeepAlive() );

        // 测试set、get方法
        options.setUseNagle( true );
        final int timeoutMs = 10 * 1000;
        assertTrue( true == options.getUseNagle() );
        options.setSocketTimeout( timeoutMs );
        assertTrue( timeoutMs == options.getSocketTimeout() );
        options.setConnectTimeout( timeoutMs );
        assertTrue( timeoutMs == options.getConnectTimeout() );
        options.setMaxAutoConnectRetryTime( timeoutMs );
        assertTrue( timeoutMs == options.getMaxAutoConnectRetryTime() );
        options.setSocketKeepAlive( false );
        assertTrue( false == options.getSocketKeepAlive() );
    }

    @AfterClass
    public void tearDown() {
        System.out.println( "the TestCase Name:" + this.getClass().getName()
                + ". the TestCase end at:"
                + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss.SSS" )
                        .format( new Date() ) );
    }
}

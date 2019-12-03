package com.sequoiadb.sync;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.Logger;

/**
 * Created by laojingtang on 17-7-24.
 */
public class TestSync11752 extends SdbTestBase {
    Logger log = Logger.getLogger( TestSync11752.class.getName() );

    @BeforeClass
    public void setup() {
        log.info( "start" );
    }

    @AfterClass
    public void teardown() {
        log.info( "teardown" );
    }

    @Test
    public void test() {
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 参数校验
        BSONObject option = new BasicBSONObject();
        int[] num = { 1, 0, -1 };
        for ( int i : num ) {
            option.put( "Deep", i );
            option.put( "Block", true );
            db.sync( option );
            option.put( "Block", false );
            db.sync( option );
        }

        option.put( "Deep", "xxx" );
        option.put( "Block", "xxx" );
        try {
            db.sync( option );
            Assert.fail( "传入非法参数不报错" );
        } catch ( BaseException e ) {
            log.info( e.toString() );
        }

    }

}

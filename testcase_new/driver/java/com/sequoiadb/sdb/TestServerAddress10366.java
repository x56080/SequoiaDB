package com.sequoiadb.sdb;

import java.net.UnknownHostException;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.IConnection;
import com.sequoiadb.net.ServerAddress;
import com.sequoiadb.testcommon.SdbTestBase;

public class TestServerAddress10366 extends SdbTestBase {
    private Sequoiadb sdb;
    private String clName = "cl10366";
    private CollectionSpace cs;

    @Test
    public void testServerAddress() {
        System.out.println( "the TestCase Name:" + this.getClass().getName()
                + ". the TestCase start at:"
                + new SimpleDateFormat( "YYYY-MM-dd HH:mm:ss.SSS" )
                        .format( new Date() ) );
        try {
            ServerAddress address = new ServerAddress( SdbTestBase.coordUrl );
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            this.sdb.setServerAddress( address );
            IConnection connection = this.sdb.getConnection();
            Assert.assertEquals( this.sdb.getServerAddress().toString(),
                    address.toString() );
            Assert.assertEquals( connection.isClosed(), false );
            Assert.assertEquals( this.sdb.isValid(), true );
            this.sdb.disconnect();
            try {
                sdb.listCollections();
                Assert.fail();
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -64 );
            }
        } catch ( UnknownHostException e ) {
            e.printStackTrace();
            Assert.fail(
                    "Sequoiadb driver TestServerAddress10366 testServerAddress error, error description:"
                            + e.getMessage() );
        } catch ( Exception e ) {
            e.printStackTrace();
            Assert.fail();
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( !this.sdb.isClosed() ) {
                if ( this.cs.isCollectionExist( clName ) ) {
                    this.cs.dropCollection( clName );
                }
                this.sdb.disconnect();
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}

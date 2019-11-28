package com.sequoiadb.basicoperation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

/**
 * Created by laojingtang on 17-9-18.
 */
public class TestLobStream12262 extends SdbTestBase {

    int[] lobSizes = { 64, 512, 1024 };
    Sequoiadb db = null;
    DBCollection cl = null;

    SimpleDateFormat sdf = new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" );

    @BeforeClass
    public void setup() {
        System.out.println( this.getClass().getName() + " begin at "
                + sdf.format( new Date() ) );
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cl = db.getCollectionSpace( SdbTestBase.csName )
                .createCollection( "lob12262" );
    }

    @AfterClass
    public void teardown() {
        db.getCollectionSpace( SdbTestBase.csName )
                .dropCollection( "lob12262" );
        System.out.println( this.getClass().getName() + " end at "
                + sdf.format( new Date() ) );
    }

    @Test
    public void testWrite() throws IOException {
        for ( int lobSize : lobSizes ) {
            DBLob lob = cl.createLob();
            byte[] bytes = new byte[ lobSize ];
            Random random = new Random();
            random.nextBytes( bytes );

            InputStream inputStream = new ByteArrayInputStream( bytes );
            lob.write( inputStream );
            lob.close();

            lob = cl.openLob( lob.getID() );
            byte[] readBytes = new byte[ lobSize ];
            lob.read( readBytes );
            Assert.assertEquals( bytes, readBytes );
        }
    }

    @Test
    public void testRead() {
        for ( int lobSize : lobSizes ) {
            DBLob lob = cl.createLob();
            byte[] bytes = new byte[ lobSize ];
            Random random = new Random();
            random.nextBytes( bytes );

            lob.write( bytes );
            lob.close();

            lob = cl.openLob( lob.getID() );
            ByteArrayOutputStream outputStream = new ByteArrayOutputStream(
                    lobSize );
            lob.read( outputStream );
            lob.close();
            Assert.assertEquals( bytes, outputStream.toByteArray() );
        }
    }
}

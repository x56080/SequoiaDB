package com.sequoiadb.test.db;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import static org.junit.Assert.assertTrue;

public class SdbConnect {

    private static Sequoiadb sdb;
    private static final String ERROR_ADDRESS_CONN = "ErrorHost:11810";

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void sdbDisconnect() {
        Sequoiadb db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        db.disconnect();
        db.disconnect();
    }

    @Test
    public void sdbConnect() {
        List<String> list = new ArrayList<String>();
        try {
            list.add("192.168.20.35:12340");
            list.add("192.168.20.36:12340");
            list.add("123:123");
            list.add("");
            list.add(":12340");
            list.add("localhost:50000");
            list.add("localhost:11810");
            list.add("localhost:12340");
            list.add(Constants.COOR_NODE_CONN);

            ConfigOptions options = new ConfigOptions();
            options.setMaxAutoConnectRetryTime(0);
            options.setConnectTimeout(10000);
            // connect
            long begin = 0;
            long end = 0;
            begin = System.currentTimeMillis();
            Sequoiadb sdb1 = new Sequoiadb(list, "", "", options);
            end = System.currentTimeMillis();
            System.out.println("Takes " + (end - begin));
            // set option and change the connect
            options.setConnectTimeout(15000);
            sdb1.changeConnectionOptions(options);
            // check
            DBCursor cursor = sdb1.getList(4, null, null, null);
            assertTrue(cursor != null);
            sdb1.disconnect();
        } catch (BaseException e) {
            SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            System.out.println("Debug info: failed to testcase at: " + df.format(new Date()));
            System.out.println("Debug info: address list is: " + list.toString());
            throw e;
        }
    }

    @Test
    public void sdbBuilderServerAddressTest() {
        // case 1: default value
        try {
            Sequoiadb db1 = Sequoiadb.builder().build();
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }

        // case 2: ""
        try {
            Sequoiadb db2 = Sequoiadb.builder().serverAddress( "" ).build();
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }

        // case 3: normal
        try ( Sequoiadb db3 = Sequoiadb.builder().serverAddress( Constants.COOR_NODE_CONN ).build() ) {
            Assert.assertTrue( db3.isValid() );
        }

        // case 4: address list
        List<String> addressList = new ArrayList<>();
        addressList.add( ERROR_ADDRESS_CONN );
        addressList.add( Constants.COOR_NODE_CONN );
        try ( Sequoiadb db4 = Sequoiadb.builder().serverAddress( addressList ).build() ) {
            Assert.assertTrue( db4.isValid() );
        }
    }

    @Test
    public void sdbBuilderConfTest() {
        long start = System.currentTimeMillis();
        try {
            ConfigOptions conf = new ConfigOptions();
            conf.setConnectTimeout( 2000 );  // 2s
            conf.setMaxAutoConnectRetryTime( 0 );

            Sequoiadb db = Sequoiadb.builder()
                    .serverAddress( ERROR_ADDRESS_CONN )  // invalid address
                    .configOptions( conf )
                    .build();
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_NET_CANNOT_CONNECT.getErrorCode(), e.getErrorCode() );
            long time = System.currentTimeMillis() - start;
            if ( time >= 3000 ) {
                Assert.fail("The elapsed time dose no match the settings, the elapsed time: " + time + "ms");
            }
        }
    }

    @Test
    public void sdbBuilderUserTest() {
        // case 1: default username and password
        UserConfig userConfig = new UserConfig();
        Assert.assertEquals( "", userConfig.getUserName() );
        Assert.assertEquals( "", userConfig.getPassword() );

        // case 2: connect by username and password
        try {
            sdb.createUser( Constants.TEST_USER_NAME, Constants.TEST_USER_PASSWORD );
            try ( Sequoiadb db = Sequoiadb.builder()
                    .serverAddress( Constants.COOR_NODE_CONN )
                    .userConfig( new UserConfig( Constants.TEST_USER_NAME, Constants.TEST_USER_PASSWORD ) )
                    .build() ) {
                Assert.assertTrue( db.isValid() );
            }
        } finally {
            sdb.removeUser( Constants.TEST_USER_NAME, Constants.TEST_USER_PASSWORD );
        }
    }
}

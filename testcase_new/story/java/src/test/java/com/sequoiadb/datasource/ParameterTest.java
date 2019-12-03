package com.sequoiadb.datasource;

import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Parameters;
import org.testng.annotations.Test;

import com.sequoiadb.base.SequoiadbDatasource;
import com.sequoiadb.base.SequoiadbOption;
import com.sequoiadb.exception.BaseException;

public class ParameterTest extends DataSourceTestBase {
    private SequoiadbDatasource datasource = null;

    @BeforeClass
    void createDataSource() {
        try {
            super.init();
            if ( datasource == null ) {
                datasource = new SequoiadbDatasource( this.coordAddr,
                        this.userName, this.password, null );
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @AfterClass
    void closeDataSource() {
        try {
            if ( null != datasource ) {
                datasource.close();
            }
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }
    }

    @Test
    void keepAliveTest() {
        try {
            // 小于checkInteval
            SequoiadbOption option = new SequoiadbOption();
            option.setKeepAliveTimeout( 5000 );
            datasource.updateDatasourceOptions( option );

            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }

        try {
            SequoiadbOption option = new SequoiadbOption();
            option.setKeepAliveTimeout( -100 );
            datasource.updateDatasourceOptions( option );

            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test
    void coordUrlTest() {
        try {
            SequoiadbDatasource datasource = new SequoiadbDatasource( null,
                    this.userName, this.password, null );
            Assert.assertTrue( false );
            datasource.close();
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }

        try {
            SequoiadbDatasource datasource = new SequoiadbDatasource( null,
                    this.userName, this.password, null, null );
            Assert.assertTrue( false );
            datasource.close();
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
        /*
         * ArrayList<String> coordAddrList = new ArrayList<String>();
         * coordAddrList.add(null); coordAddrList.add(""); try{
         * SequoiadbDatasource datasource = new
         * SequoiadbDatasource(coordAddrList, this.userName, this.password,
         * null, null); Assert.assertTrue(false); }catch(BaseException e){
         * super.judegeErrCode("SDB_INVALIDARG", e.getErrorCode()); }
         */
    }

    @DataProvider(name = "val-provider")
    public Object[][] optionVal() {
        int negative = -1;
        int minVal = 0;
        int maxVal = 500;
        return new Object[][] { { negative }, { minVal }, { maxVal }, };
    }

    @Test(dataProvider = "val-provider")
    void deltaIncCountTest( int val ) {
        try {
            if ( val > 0 )
                return;
            SequoiadbOption option = new SequoiadbOption();
            option.setDeltaIncCount( val );
            datasource.updateDatasourceOptions( option );

            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test(dataProvider = "val-provider")
    void maxIdelCountTest( int val ) {
        try {
            if ( val >= 0 )
                return;
            SequoiadbOption option = new SequoiadbOption();
            option.setMaxIdleCount( val );
            datasource.updateDatasourceOptions( option );

            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test(dataProvider = "val-provider")
    void maxCountTest( int val ) {
        if ( val >= 0 )
            return;
        try {
            SequoiadbOption option = new SequoiadbOption();
            option.setMaxCount( val );
            datasource.updateDatasourceOptions( option );

            Assert.assertFalse( true );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test(dataProvider = "val-provider")
    void syncCoordIntervalTest( int val ) {
        if ( val >= 0 )
            return;
        try {
            SequoiadbOption option = new SequoiadbOption();
            option.setSyncCoordInterval( val );
            datasource.updateDatasourceOptions( option );

            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test
    void addCoordTest() {
        try {
            datasource.addCoord( this.coordAddr );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

        try {
            datasource.addCoord( "" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }

        try {
            datasource.addCoord( null );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test
    void delCoordTest() {
        try {
            datasource.removeCoord( this.coordAddr );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

        try {
            datasource.removeCoord( this.coordAddr );
        } catch ( BaseException e ) {
            Assert.assertFalse( true, e.getMessage() );
        }

        try {
            datasource.removeCoord( "" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }

        try {
            datasource.removeCoord( null );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }
}

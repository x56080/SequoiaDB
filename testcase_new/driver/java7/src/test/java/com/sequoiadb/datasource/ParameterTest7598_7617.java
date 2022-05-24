package com.sequoiadb.datasource;


import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;

public class ParameterTest7598_7617 extends DataSourceTestBase {
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
    void keepAliveTest7598() {
        try {
            // 小于checkInteval
            DatasourceOptions option = new DatasourceOptions();
            option.setKeepAliveTimeout( 5000 );
            datasource.updateDatasourceOptions( option );

            Assert.fail( "must throw exception" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }

        try {
            DatasourceOptions option = new DatasourceOptions();
            option.setKeepAliveTimeout( -100 );
            datasource.updateDatasourceOptions( option );

            Assert.fail( "must throw exception" );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test
    void coordUrlTest7607() {
        try {
            SequoiadbDatasource datasource = new SequoiadbDatasource( null,
                    this.userName, this.password, null );
            Assert.fail( "must throw exception" );
            datasource.close();
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @DataProvider(name = "val-provider")
    public Object[][] optionVal() {
        int negative = -1;
        int minVal = 0;
        int maxVal = 500;
        return new Object[][] { { negative }, { minVal }, { maxVal }, };
    }

    @Test(dataProvider = "val-provider")
    void deltaIncCountTest7609( int val ) {
        try {
            if ( val > 0 )
                return;
            DatasourceOptions option = new DatasourceOptions();
            option.setDeltaIncCount( val );
            datasource.updateDatasourceOptions( option );

            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test(dataProvider = "val-provider")
    void maxIdelCountTest7610( int val ) {
        try {
            if ( val >= 0 )
                return;
            DatasourceOptions option = new DatasourceOptions();
            option.setMaxIdleCount( val );
            datasource.updateDatasourceOptions( option );

            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test(dataProvider = "val-provider")
    void maxCountTest7611( int val ) {
        if ( val >= 0 )
            return;
        try {
            DatasourceOptions option = new DatasourceOptions();
            option.setMaxCount( val );
            datasource.updateDatasourceOptions( option );

            Assert.assertFalse( true );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test(dataProvider = "val-provider")
    void syncCoordIntervalTest7612( int val ) {
        if ( val >= 0 )
            return;
        try {
            DatasourceOptions option = new DatasourceOptions();
            option.setSyncCoordInterval( val );
            datasource.updateDatasourceOptions( option );

            Assert.assertTrue( false );
        } catch ( BaseException e ) {
            super.judegeErrCode( "SDB_INVALIDARG", e.getErrorCode() );
        }
    }

    @Test
    void addCoordTest7613_7614() {
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
    void delCoordTest7616_7617() {
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

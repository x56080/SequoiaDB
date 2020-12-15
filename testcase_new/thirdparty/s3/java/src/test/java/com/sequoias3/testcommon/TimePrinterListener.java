package com.sequoias3.testcommon;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import org.testng.ITestResult;
import org.testng.TestListenerAdapter;

import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * Created by laojingtang on 17-11-23.
 */
public class TimePrinterListener extends TestListenerAdapter {
    private static final String errorCode = "DBError";

    // transactions snapshot
    private static String transSnapshot() {
        DBCursor cursor = null;
        StringBuilder builder = new StringBuilder();
        builder.append( "[" );
        try ( Sequoiadb db = new Sequoiadb( S3TestBase.getDefaultCoordUrl(), "",
                "" ) ;) {
            cursor = db.getSnapshot( 9, "", "", "" );
            while ( cursor.hasNext() ) {
                builder.append( cursor.getNext().toString() + ",\n" );
            }
        } catch ( Exception e ) {
            System.out.println( "snapshot 9 failed,coord = "
                    + S3TestBase.getDefaultCoordUrl() );
            e.printStackTrace();
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
        builder.append( "]" );
        return builder.toString();
    }

    @Override
    public void onConfigurationSuccess( ITestResult itr ) {
        super.onConfigurationSuccess( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
            dbMsgEndTime( itr );
        }
    }

    @Override
    public void onConfigurationFailure( ITestResult itr ) {
        super.onConfigurationFailure( itr );
        Throwable throwable = itr.getThrowable();
        if ( throwable != null && throwable.getMessage() != null
                && throwable.getMessage().contains( errorCode ) ) {
            System.out.println( getCurTimeStr() + " "
                    + itr.getTestClass().getRealClass().getName()
                    + ":transaction snapshot:" + transSnapshot() );
        }
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
            dbMsgEndTime( itr );
        }
    }

    @Override
    public void onConfigurationSkip( ITestResult itr ) {
        super.onConfigurationSkip( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
            dbMsgEndTime( itr );
        }
    }

    @Override
    public void beforeConfiguration( ITestResult tr ) {
        super.beforeConfiguration( tr );
        if ( tr.getMethod().isBeforeTestConfiguration() ) {
            S3TestBase.setRunGroup(
                    tr.getTestClass().getXmlTest().getIncludedGroups() );
        }
        if ( tr.getMethod().isBeforeClassConfiguration() ) {
            printBeginTime( tr );
            dbMsgBeginTime( tr );
        }
    }

    @Override
    public void onTestFailure( ITestResult tr ) {
        super.onTestFailure( tr );
        Throwable throwable = tr.getThrowable();
        if ( throwable != null && throwable.getMessage() != null
                && throwable.getMessage().contains( errorCode ) ) {
            System.out.println( getCurTimeStr() + " "
                    + tr.getTestClass().getRealClass().getName()
                    + ":transaction snapshot:" + transSnapshot() );
        }
    }

    private void printBeginTime( ITestResult tr ) {
        System.out.println(
                getCurTimeStr() + "\tbegin: " + getTestMethodName( tr ) );
    }

    private void printEndTime( ITestResult tr ) {
        System.out.println(
                getCurTimeStr() + "\tend  : " + getTestMethodName( tr ) );
    }

    private String getTestMethodName( ITestResult tr ) {
        return tr.getTestClass().getRealClass().getName();
    }

    private String getCurTimeStr() {
        return new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss:S" )
                .format( new Date() );
    }

    private void dbMsgBeginTime( ITestResult tr ) {
        try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" ) ;) {
            sdb.msg( getCurTimeStr() + "\tBegin testcase: "
                    + getTestMethodName( tr ) );
        } catch ( BaseException e ) {
            e.printStackTrace();
        }
    }

    private void dbMsgEndTime( ITestResult tr ) {
        try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" ) ;) {
            sdb.msg( getCurTimeStr() + "\tEnd testcase: "
                    + getTestMethodName( tr ) );
        } catch ( BaseException e ) {
            e.printStackTrace();
        }
    }
}

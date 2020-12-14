package com.mongodb.utils;

import org.apache.log4j.Logger;
import org.testng.ITestResult;
import org.testng.TestListenerAdapter;

/**
 * Created by laojingtang on 17-11-23.
 */
public class TimePrinterListener extends TestListenerAdapter {
    private static final Logger logger = Logger
            .getLogger( TimePrinterListener.class );

    @Override
    public void onConfigurationSuccess( ITestResult itr ) {
        super.onConfigurationSuccess( itr );
        if ( itr.getName().equals( "springTestContextAfterTestClass" ) ) {
            printEndTime( itr );
        }
    }

    @Override
    public void onConfigurationFailure( ITestResult itr ) {
        super.onConfigurationFailure( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
        }
    }

    @Override
    public void onConfigurationSkip( ITestResult itr ) {
        super.onConfigurationSkip( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
        }
    }

    @Override
    public void beforeConfiguration( ITestResult itr ) {
        super.beforeConfiguration( itr );
        if ( itr.getName().equals( "springTestContextBeforeTestClass" ) ) {
            printBeginTime( itr );
        }
    }

    private void printBeginTime( ITestResult itr ) {
        logger.info( "\tBegin testcase: java-mongo-"
                + MongodbTestBase.javaMongoVersion + " "
                + getTestMethodName( itr ) );
    }

    private void printEndTime( ITestResult itr ) {
        logger.info(
                "\tEnd testcase: java-mongo-" + MongodbTestBase.javaMongoVersion
                        + " " + getTestMethodName( itr ) );
    }

    private String getTestMethodName( ITestResult itr ) {
        return itr.getTestClass().getRealClass().getName();
    }
}

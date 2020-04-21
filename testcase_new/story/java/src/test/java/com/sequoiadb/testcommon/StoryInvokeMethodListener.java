package com.sequoiadb.testcommon;

import org.testng.IInvokedMethod;
import org.testng.IInvokedMethodListener;
import org.testng.ITestResult;

/**
 * @FileName
 * @Author luweikang
 * @Date 2020-4-21
 * @Version 1.00
 */
public class StoryInvokeMethodListener implements IInvokedMethodListener {
    @Override
    public void beforeInvocation( IInvokedMethod iInvokedMethod,
            ITestResult iTestResult ) {
    }

    @Override
    public void afterInvocation( IInvokedMethod iInvokedMethod,
            ITestResult iTestResult ) {
        if ( iTestResult.getStatus() == ITestResult.FAILURE ) {
            iTestResult.getTestContext().getSuite().getSuiteState().failed();
        }
    }
}

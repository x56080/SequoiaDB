/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:FaultMakeTask.java
 * 
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.task;

import java.util.Random;

import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.Fault;
import com.sequoiadb.fault.FaultWrapper;

public class FaultMakeTask extends Task {

    public static final String MAKE_RESULT = "MakeResult";
    public static final String RESTORE_RESULT = "RestoreResult";

    private FaultWrapper faultInstance;
    private final int MilliSecondsPerSecond = 1000;
    private int duration;

    public FaultMakeTask(Fault instance, int maxDlay, int duration, int checkTimes) {
        super(instance.getName(), maxDlay);
        // TODO Auto-generated constructor stub
        faultInstance = new FaultWrapper(instance, checkTimes);
        this.duration = duration;
    }

    @SuppressWarnings("static-access")
    public void run() {
        Random random = new Random();
        try {
            Thread.currentThread()
                    .sleep(random.nextInt(super.randomStartMaxDuration * MilliSecondsPerSecond));
        }
        catch (Exception e) {
            // TODO Auto-generated catch block
        }
        try {
            faultInstance.make();
        }
        catch (ReliabilityException e) {
            exception = e;
        }
        try {
            Thread.currentThread().sleep(duration * MilliSecondsPerSecond);
        }
        catch (Exception e) {
            // TODO Auto-generated catch block
        }
        try {
            faultInstance.restore();
        }
        catch (ReliabilityException e) {
            exception = e;
        }

    }

    public void addDependsTask(OperateTask task) {
        faultInstance.addDependsTask(task);
    }

    public void removeDependsTask(OperateTask task) {
        faultInstance.removeDependsTask(task);
    }

    @Override
    public void init() throws ReliabilityException {
        faultInstance.init();
    }

    @Override
    public void fini() throws ReliabilityException {
        faultInstance.fini();
    }

    public Fault getFaultInstance() {
        return faultInstance;
    }
}

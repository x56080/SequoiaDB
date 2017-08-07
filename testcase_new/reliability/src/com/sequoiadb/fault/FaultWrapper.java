/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:FaultWrapper.java
 * 
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.fault;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.exception.FaultException;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;

public class FaultWrapper extends Fault {
    private Fault instance;
    private int checkTimes = 3;
    private List<OperateTask> taskSet = new ArrayList<OperateTask>();
    OperateTask.faultStatus status;

    private void handleException(FaultException e) {
        status = OperateTask.faultStatus.EXCEPTION;
    }

    public FaultWrapper(Fault instance) {
        super(instance.getName());
        this.instance = instance;
    }

    public FaultWrapper(Fault instance, int checkTimes) {
        super(instance.getName());
        this.instance = instance;
        this.checkTimes = checkTimes;
    }

    public void addDependsTask(OperateTask task) {
        taskSet.add(task);
    }

    public void removeDependsTask(OperateTask task) {
        int pos = 0;
        for (; pos < taskSet.size(); ++pos) {
            if (task.getName().equals(taskSet.get(pos).getName())) {
                break;
            }
        }
        taskSet.remove(pos);
    }

    public void make() throws FaultException {
        FaultException exception = null;
        Date date = Calendar.getInstance().getTime();
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        System.out.println("make " + instance.getName() + " at " + sdf.format(date));

        try {
            instance.make();
            if (status != OperateTask.faultStatus.EXCEPTION) {
                if (checkMakeResult()) {
                    date = Calendar.getInstance().getTime();
                    status = OperateTask.faultStatus.MAKESUCCESS;
                    sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                    System.out.println(
                            "make " + instance.getName() + " success at " + sdf.format(date));
                }
                else {
                    System.out.println(
                            "make " + instance.getName() + " failure at " + sdf.format(date));
                }
            }
        }
        catch (FaultException e) {
            handleException(e);
            exception = e;
        }

        // 通知依赖的任务
        for (OperateTask task : taskSet) {
            BSONObject bson = new BasicBSONObject();
            bson.put(FaultMakeTask.MAKE_RESULT, status);
            task.faultNotify(bson);
        }

        if (exception != null) {
            throw exception;
        }

    }

    public boolean checkMakeResult() throws FaultException {
        boolean checkResult = false;
        status = OperateTask.faultStatus.MAKEFAILURE;
        for (int i = 0; i < checkTimes; ++i) {
            try {
                checkResult = instance.checkMakeResult();
            }
            catch (FaultException e) {
                if (i >= checkTimes - 1) {
                    throw e;
                }
            }
            if (checkResult) {
                status = OperateTask.faultStatus.MAKESUCCESS;
                break;
            }
            try {
                Thread.sleep(500);
            }
            catch (Exception e) {
                // ignore
            }
        }
        return checkResult;
    }

    public void restore() throws FaultException {
        FaultException exception = null;
        if (status != OperateTask.faultStatus.MAKESUCCESS) {
            return;
        }

        Date date = Calendar.getInstance().getTime();
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        System.out.println("restore " + instance.getName() + " at " + sdf.format(date));
        try {
            instance.restore();
            if (status != OperateTask.faultStatus.EXCEPTION) {
                if (checkRestoreResult()) {
                    date = Calendar.getInstance().getTime();
                    sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                    System.out.println(
                            "restore " + instance.getName() + " success at " + sdf.format(date));
                }
                else {
                    System.out.println(
                            "restore " + instance.getName() + " failure at " + sdf.format(date));
                    status = OperateTask.faultStatus.RESTOREFAILURE;
                }
            }
        }
        catch (FaultException e) {
            handleException(e);
            exception = e;
        }

        // 通知依赖的任务
        for (OperateTask task : taskSet) {
            BSONObject bson = new BasicBSONObject();
            bson.put(FaultMakeTask.RESTORE_RESULT, status);
            task.faultNotify(bson);
        }

        if (exception != null) {
            throw exception;
        }
    }

    public boolean checkRestoreResult() throws FaultException {
        boolean checkResult = false;
        status = OperateTask.faultStatus.RESTOREFAILURE;
        for (int i = 0; i < checkTimes; ++i) {
            checkResult = instance.checkRestoreResult();
            if (checkResult) {
                status = OperateTask.faultStatus.RESTORESUCESS;
                break;
            }
        }
        return checkResult;
    }

    @Override
    public void init() throws FaultException {
        instance.init();
    }

    @Override
    public void fini() throws FaultException {
        instance.fini();
    }

}

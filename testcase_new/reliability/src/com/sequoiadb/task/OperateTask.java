/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:OperateTask.java
 * 
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.task;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import org.bson.BSONObject;

import com.sequoiadb.exception.FaultException;
import com.sequoiadb.exception.ReliabilityException;

public abstract class OperateTask extends Task {
    public enum faultStatus {
        INIT,
        MAKESUCCESS,
        MAKEFAILURE,
        RESTORESUCESS,
        RESTOREFAILURE,
        EXCEPTION
    };

    private TaskMgr mgr = null;

    // 暂时未使用
    private static final int defaultDuration = 5;

    public OperateTask(String name) {
        super(name, defaultDuration);
    }

    public OperateTask() {
        this.setName(this.getClass().getSimpleName());
    }

    public void setMgr(TaskMgr mgr) {
        this.mgr = mgr;
    }

    /**
     * 当故障make或restore完毕将会调用此函数，若OperateTask子类需要覆盖该方法时，应该要首先调用父类faultNotify方法，
     * 父类的faultNotify实现了当故障构造或恢复失败抛出异常，使TaskMgr感知到故障线程执行失败
     * 
     * @param status
     *            key:FaultMakeTask.MAKE_RESULT,FaultMakeTask.RESTORE_RESULT
     *            value:OperateTask.status
     */
    public void faultNotify(BSONObject status) throws FaultException {
        OperateTask.faultStatus mk = (faultStatus) status.get(FaultMakeTask.MAKE_RESULT);
        OperateTask.faultStatus rt = (faultStatus) status.get(FaultMakeTask.RESTORE_RESULT);
        if (mk == OperateTask.faultStatus.MAKEFAILURE) {
            throw new FaultException(mk.toString());
        }
        if (rt == OperateTask.faultStatus.RESTOREFAILURE) {
            throw new FaultException(rt.toString());
        }
    }

    public abstract void exec() throws Exception;

    public void run() {
        setStatus(Task.TaskStatus.TASKSTART);
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        Date date = Calendar.getInstance().getTime();
        System.out.println("Thread '" + this.getName() + "' run at " + sdf.format(date));
        try {
            exec();
        }
        catch (Exception e) {
            if (e instanceof InterruptedException) {
                setStatus(Task.TaskStatus.TASKINTERRUPT);
            }
            else {
                setStatus(Task.TaskStatus.TASKTHROWEXCEPTION);
                exception = new ReliabilityException(e);
                exception.setStackTrace(e.getStackTrace());
            }
        }
        if (exception == null) {
            setStatus(Task.TaskStatus.TASKSTOP);
        }
        date = Calendar.getInstance().getTime();
        System.out.println("Thread '" + this.getName() + "' end at " + sdf.format(date));
        // mgr.Done(this);
    }

    public Task getTaskByName(String name) {
        return mgr.getTaskByName(name);
    }

}

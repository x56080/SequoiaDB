/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:Task.java
 * 
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.task;

import com.sequoiadb.exception.ReliabilityException;

public abstract class Task extends Thread {
    public enum TaskStatus {
        TASKSTART,
        TASKINTERRUPT,
        TASKTHROWEXCEPTION,
        TASKSTOP
    };

    protected int randomStartMaxDuration;
    protected TaskStatus status;
    protected ReliabilityException exception;

    public Task() {
        super();
    }

    public Task(String name, int maxDuration) {
        this.setName(name);
        this.randomStartMaxDuration = maxDuration;
    }

    public ReliabilityException getException() {
        try {
            this.join();
        }
        catch (InterruptedException e) {
            // TODO Auto-generated catch block
        }
        return exception;
    }

    public void init() throws ReliabilityException {

    }
    
    public void check() throws ReliabilityException {
        
    }

    public void fini() throws ReliabilityException {

    }

    public void setStatus(TaskStatus status) {
        this.status = status;
    }

    public TaskStatus getStatus() {
        return this.status;
    }

    /**
     * 等待某一阶段的任务完成 注：只能用于线程方法中
     */
    public void waitComplete() {
        synchronized (this) {
            try {
                this.wait();
            }
            catch (InterruptedException e) {
                status = TaskStatus.TASKINTERRUPT;
            }
        }
    }

    /**
     * 通知等待的任务，当前任务某一阶段的任务已经完成 注：只能用于线程方法中
     */
    public void notifyComplete() {
        synchronized (this) {
            this.notify();
        }
    }

    /**
     * 通知所有等待的任务，当前任务某一阶段的任务已经完成 注：只能用于线程方法中
     */
    public void notifyAllComplete() {
        synchronized (this) {
            this.notifyAll();
        }
    }

    public boolean isSuccess() {
        return this.getException() == null;
    }

    public String getErrorMsg() {
        ReliabilityException exception = this.getException();
        String reStr = new String();
        if (exception == null) {
            return reStr;
        }

        // errorMsg
        reStr += getName();
        reStr += " ErrMsg: ";
        reStr += exception.getMessage();
        reStr += "\r\n";

        reStr += "StackTrace: \r\n";
        // stackMsg
        StringBuffer stackBuffer = new StringBuffer();
        StackTraceElement[] stackElements = exception.getStackTrace();
        for (int j = 0; j < stackElements.length; j++) {
            stackBuffer.append(stackElements[j].toString()).append("\r\n");
        }
        reStr += stackBuffer.toString();
        return reStr;
    }
}

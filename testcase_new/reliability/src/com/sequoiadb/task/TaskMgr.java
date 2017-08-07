/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:TaskMgr.java
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.task;

import java.util.HashMap;
import java.util.Map;

import com.sequoiadb.exception.ReliabilityException;

public class TaskMgr {
    private Map<String, Task> taskSet = new HashMap<String, Task>();
    FaultMakeTask faultMakeTask;

    public TaskMgr(FaultMakeTask faultMakeTask) {
        this.faultMakeTask = faultMakeTask;
        taskSet.put(faultMakeTask.getName(), faultMakeTask);
    }

    public TaskMgr() {

    }

    /**
     * add by jt
     * @param faultMakeTask
     * @param operateTasks
     */
    public TaskMgr(FaultMakeTask faultMakeTask, OperateTask... operateTasks) {
        this(faultMakeTask);
        for (OperateTask task : operateTasks) {
            addTask(task);
        }
    }

    /**
     * add by jt.
     *
     * @param faultMakeTask
     * @return
     */

    public static TaskMgr getTaskMgr(FaultMakeTask faultMakeTask, OperateTask... operateTasks) {
        TaskMgr taskMgr = new TaskMgr(faultMakeTask);
        for (OperateTask task : operateTasks) {
            taskMgr.addTask(task);
        }
        return taskMgr;
    }

    /**
     * @param task 任务
     */
    public void addTask(String taskClassName) {
        OperateTask task = OperateTaskFactory.newTask(taskClassName, this);
        if (task == null) {
            return;
        }
        if (!taskSet.containsKey(task.getName())) {
            taskSet.put(task.getName(), task);
        }
        if (faultMakeTask != null) {
            faultMakeTask.addDependsTask((OperateTask) task);
        }
    }

    public void addTask(Task task) {
        if (!taskSet.containsKey(task.getName())) {
            taskSet.put(task.getName(), task);
        }
        if (faultMakeTask != null) {
            faultMakeTask.addDependsTask((OperateTask) task);
        }
    }

    /**
     * @param task 任务
     */
    public void removeTask(Task task) {
        if (taskSet.containsKey(task.getName())) {
            taskSet.remove(task.getName());
        }

        if (faultMakeTask != null) {
            faultMakeTask.removeDependsTask((OperateTask) task);
        }
    }

    /**
     * @return 所有任务初始化成功，返回true，任一任务初始化失败，则返回false
     * @throws ReliabilityException
     */
    public void init() throws ReliabilityException {
        for (Map.Entry<String, Task> entry : taskSet.entrySet()) {
            entry.getValue().init();

        }
    }

    public void start() {
        for (Map.Entry<String, Task> entry : taskSet.entrySet()) {
            entry.getValue().start();
        }
    }
    
    public void join() {
        for (Map.Entry<String, Task> entry : taskSet.entrySet()) {
            try {
                entry.getValue().join();
            } catch (InterruptedException e) {
                // ignore
            }
        }
    }
    
    public void check() throws ReliabilityException {
        if (!this.isAllSuccess()) {
            throw new ReliabilityException(this.getErrorMsg());
        }
        for (Map.Entry<String, Task> entry : taskSet.entrySet()) {
            try {
                entry.getValue().check();
            } catch (ReliabilityException e) {
                throw e;
            }
        }
    }

    /**
     * @return 所有任务反初始化成功，返回true，任一任务反初始化失败，则返回false
     * @throws ReliabilityException
     */
    public boolean fini() throws ReliabilityException {
        for (Map.Entry<String, Task> entry : taskSet.entrySet()) {
            entry.getValue().fini();
        }
        return true;
    }

    /**
     * @param name 任务名
     * @return 返回对应任务名的任务，不存在返回null
     */
    public Task getTaskByName(String name) {
        if (taskSet.containsKey(name)) {
            return taskSet.get(name);
        }

        return null;
    }

    /**
     * @param task 执行完成的任务
     */
    public void Done(Task task) {
        /*
         * if ( task.getStatus() == Task.TaskStatus.TASKTHROWEXCEPTION ) { for (
         * Entry< String, Task > entry : taskSet.entrySet() ) { if (
         * !task.getName().equals( entry.getKey() ) || task.getClass().equals(
         * FaultMakeTask.class ) ) { entry.getValue().interrupt() ; } } }
         */
    }

    public Map<String, ReliabilityException> getExceptions() {
        HashMap<String, ReliabilityException> map = new HashMap<String, ReliabilityException>();
        for (Map.Entry<String, Task> entry : taskSet.entrySet()) {
            if (entry.getValue().getException() != null) {
                map.put(entry.getKey(), entry.getValue().getException());
            }
        }
        return map;
    }

    public boolean isAllSuccess() {
        return getExceptions().isEmpty();
    }

    public String getErrorMsg() {
        String reStr = new String();
        for (Map.Entry<String, Task> entry : taskSet.entrySet()) {
            reStr += entry.getValue().getErrorMsg();
        }
        return reStr;
    }

    public void clear() {
        taskSet.clear();
    }

    /**
     * 顺序调用init(),start(),join(),fini()
     *
     * @throws ReliabilityException
     */
    public void execute() throws ReliabilityException {
        init();
        start();
        join();
        fini();
    }

}

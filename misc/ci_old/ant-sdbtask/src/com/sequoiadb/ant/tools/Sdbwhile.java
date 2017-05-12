package com.sequoiadb.ant.tools;


import org.apache.tools.ant.Task;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Target;
import org.apache.tools.ant.types.DirSet;
import org.apache.tools.ant.types.FileSet;
import org.apache.tools.ant.types.Path;
import org.apache.tools.ant.util.TaskLogger;

import java.util.Collection;
import java.util.Iterator;
import java.util.Map;
/***
 * Task definition for the for task.  This is based on
 * the foreach task but takes a sequential element
 * instead of a target and only works for ant >= 1.6Beta3
 * @author Peter Reilly
 */
public class Sdbwhile extends Task  {

    private ForDelegate delegate;
    private boolean isForeverIsSet = false;

    /**
     * Creates a new <code>For</code> instance.
     * This checks if the ant version is correct to run this task.
     */
    public Sdbwhile() {
        this.delegate = new ForDelegate();
    }

    public void setProject(Project project)
    {
        super.setProject(project);
        delegate.setProject(project);
    }

    public void setOwningTarget(Target target)
    {
        super.setOwningTarget(target);
        delegate.setOwningTarget(target);
    }

    /**
     * Attribute whether to execute the loop in parallel or in sequence.
     * @param parallel if true execute the tasks in parallel. Default is false.
     */
    public void setParallel(boolean parallel) {
        delegate.setParallel(parallel);
    }

    /***
     * Set the maximum amount of threads we're going to allow
     * to execute in parallel
     * @param threadCount the number of threads to use
     */
    public void setThreadCount(int threadCount) {
        delegate.setThreadCount(threadCount);
    }

    /**
     * Set the trim attribute.
     *
     * @param trim if true, trim the value for each iterator.
     */
    public void setTrim(boolean trim) {
        delegate.setTrim(trim);
    }

    /**
     * Set the list attribute.
     *
     * @param list a list of delimiter separated tokens.
     */
    public void setList(String list) {
        delegate.setList(list);
    }
    
    /**
     * Set the keepgoing attribute, indicating whether we
     * should stop on errors or continue heedlessly onward.
     *
     * @param keepgoing a boolean, if <code>true</code> then we act in
     *                  the keepgoing manner described.
     */
    public void setIsForever(boolean isForever){
    	this.isForeverIsSet = true;
    	delegate.setIsForever(isForever);
    }
    public void setKeepgoing(boolean keepgoing) {
    	if(this.isForeverIsSet)
    		delegate.setKeepgoing(false);
    	else
    		delegate.setKeepgoing(keepgoing);
    }
    

    /**
     * Set the delimiter attribute.
     *
     * @param delimiter the delimiter used to separate the tokens in
     *        the list attribute. The default is ",".
     */
    public void setDelimiter(String delimiter) {
        delegate.setDelimiter(delimiter);
    }

    /**
     * Set the param attribute.
     * This is the name of the macrodef attribute that
     * gets set for each iterator of the sequential element.
     *
     * @param param the name of the macrodef attribute.
     */
    public void setParam(String param) {
        delegate.setParam(param);
    }

    /**
     * This is a path that can be used instread of the list
     * attribute to interate over. If this is set, each
     * path element in the path is used for an interator of the
     * sequential element.
     *
     * @param path the path to be set by the ant script.
     */
    public void addConfigured(Path path) {
        delegate.addConfigured(path);
    }

    /**
     * This is a path that can be used instread of the list
     * attribute to interate over. If this is set, each
     * path element in the path is used for an interator of the
     * sequential element.
     *
     * @param path the path to be set by the ant script.
     */
    public void addConfiguredPath(Path path) {
        delegate.addConfiguredPath(path);
    }

    /**
     * @return a MacroDef#NestedSequential object to be configured
     */
    public Object createSequential() {
        return delegate.createSequential();
    }

    /**
     * Run the for task.
     * This checks the attributes and nested elements, and
     * if there are ok, it calls doTheTasks()
     * which constructes a macrodef task and a
     * for each interation a macrodef instance.
     */
    public void execute() {
        delegate.setLogger(new TaskLogger(this));
        delegate.execute();
    }

    /**
     * Add a Map, iterate over the values
     *
     * @param map a Map object - iterate over the values.
     */
    public void add(Map map) {
        delegate.add(map);
    }

    /**
     * Add a fileset to be iterated over.
     *
     * @param fileset a <code>FileSet</code> value
     */
    public void add(FileSet fileset) {
        delegate.add(fileset);
    }

    /**
     * Add a fileset to be iterated over.
     *
     * @param fileset a <code>FileSet</code> value
     */
    public void addFileSet(FileSet fileset) {
        delegate.addFileSet(fileset);
    }

    /**
     * Add a dirset to be iterated over.
     *
     * @param dirset a <code>DirSet</code> value
     */
    public void add(DirSet dirset) {
        delegate.add(dirset);
    }

    /**
     * Add a dirset to be iterated over.
     *
     * @param dirset a <code>DirSet</code> value
     */
    public void addDirSet(DirSet dirset) {
        delegate.addDirSet(dirset);
    }

    /**
     * Add a collection that can be iterated over.
     *
     * @param collection a <code>Collection</code> value.
     */
    public void add(Collection collection) {
        delegate.add(collection);
    }

    /**
     * Add an iterator to be iterated over.
     *
     * @param iterator an <code>Iterator</code> value
     */
    public void add(Iterator iterator) {
        delegate.add(iterator);
    }

    /**
     * Add an object that has an Iterator iterator() method
     * that can be iterated over.
     *
     * @param obj An object that can be iterated over.
     */
    public void add(Object obj) {
        delegate.add(obj);
    }
}


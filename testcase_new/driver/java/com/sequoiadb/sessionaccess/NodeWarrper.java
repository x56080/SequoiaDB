package com.sequoiadb.sessionaccess;

/**
 * Created by laojingtang on 18-1-20.
 */

class NodeWarrper {
    String nodeName;
    int instenceid;
    boolean isMaster = false;

    public String getNodeName() {
        return nodeName;
    }

    public NodeWarrper setNodeName(String nodeName) {
        this.nodeName = nodeName;
        return this;
    }

    public int getInstenceid() {
        return instenceid;
    }

    public NodeWarrper setInstenceid(int instenceid) {
        this.instenceid = instenceid;
        return this;
    }

    public boolean isMaster() {
        return isMaster;
    }

    public NodeWarrper setMaster(boolean master) {
        isMaster = master;
        return this;
    }
}

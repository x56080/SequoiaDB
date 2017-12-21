/**
 * 通过nodename获取 GroupID、GroupName、NodeID
 * 备注：协调节点除外
 * @param nodename
 * @constructor
 */
function InfoByNodeName(nodename) {

    this.HostName = nodename.split(":")[0];
    this.svcname = nodename.split(":")[1];
    this.dataGroup = commGetGroups(db, true, "", true, true, true).concat(commGetGroups(db, true, "SYSCatalogGroup", false, true, true));
    /**
     * 获取GroupID
     * @returns GroupID
     */
    this.getGroupIDByNodeName = function () {
        for (var i = 0; i < this.dataGroup.length; i++) {
            for (var j = 1; j < this.dataGroup[i].length; j++) {
                // println(this.dataGroup[i][j].HostName +"=="+ this.HostName);
                // println(this.dataGroup[i][j].svcname +"=="+ this.svcname);
                if (this.dataGroup[i][j].HostName == this.HostName && this.dataGroup[i][j].svcname == this.svcname) {
                    return this.dataGroup[i][0].GroupID;
                }
            }
        }
        return null;
    }

    /**
     * 获取GroupName
     * @returns GroupName
     */
    this.getGroupNameByNodeName = function () {
        for (var i = 0; i < this.dataGroup.length; i++) {
            for (var j = 1; j < this.dataGroup[i].length; j++) {
                if (this.dataGroup[i][j].HostName == this.HostName && this.dataGroup[i][j].svcname == this.svcname) {
                    return this.dataGroup[i][0].GroupName;
                }
            }
        }
        return null;
    }

    /**
     * 获取NodeId
     * @returns NodeId
     */
    this.getNodeIdByNodeName = function () {
        for (var i = 0; i < this.dataGroup.length; i++) {
            for (var j = 1; j < this.dataGroup[i].length; j++) {
                if (this.dataGroup[i][j].HostName == this.HostName && this.dataGroup[i][j].svcname == this.svcname) {
                    return this.dataGroup[i][j].NodeID;
                }
            }
        }
        return null;
    }
}



/**
 * 通过url获取连接
 * @param url 例：sdbserver01:11820
 */
function getConn(url) {
    var conn = null;
    try {
        conn = new Sdb(url);
    } catch (e) {
        throw buildException("connection to sdb by url[" + url + "] error", e);
    }
    return conn;
}

function isSystemEDU( context )
{
   var nodeName = context.NodeName ;
   var sessionId = context.SessionID ;
   var cursor = db.snapshot( SDB_SNAP_SESSIONS, { NodeName: nodeName, SessionID: sessionId } ) ;
   var eduType = cursor.next().toObj().Type ;
   var systemEduTypes = [ "TCPListener", "RestListener", "ReplReader", "LogWriter",
                          "LogArchiveMgr", "DpsRollback", "ShardReader", "Cluster",
                          "ClusterShard", "ClusterLogNotify", "CatalogMgr",
                          "CatalogNetwork", "CoordNetwork", "CoordMgr", "OMManager",
                          "OMNet", "SyncClockWorker", "PipeListener", "FAPListener",
                          "DBMonitor", "RtnNetwork", "SignalTest", "SeAdapter", "SeIndexerReader",
                          "SeService" ] ;
   println( "session: " + nodeName + ":" + sessionId + ", edu type is " + eduType ) ;
   return systemEduTypes.indexOf( eduType ) !== -1 ;
}
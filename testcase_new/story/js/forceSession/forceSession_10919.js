/**************************************
 * @Description: 连接coord节点指定sessionID和options参数终止会话  测试用例seqDB-10919
 * @author: ouyangzhongnan
 * @Date: 2017-01-04
 * @RunDemo:
 * sdb -f "func.js,commlib.js,forceSession_10919.js" -e "var CHANGEDPREFIX='PREFIX';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/
/*
 操作步骤：
 1、连接coord节点
 2、通过list()获取当前集群session（如db.list( SDB_LIST_CONTEXTS )）
 3、执行db.forceSession(sessionID,options)终止会话，分别验证如下场景：
 a、options指定NodeID
 b、options指定HostName
 c、options指定svcname
 d、options指定两个参数，如NodeID和hostName
 e、options指定三个参数
 4、查看指定session是否存在
 期望结果：
 1、a、d、e场景，指定session被终止，查看session已不存在
 2、b、c场景，节点上存在指定sessionID被终止，部分节点未匹配到sessionID，系统返回对应错误信息
 [指定session依然存在，2017-01-18改了代码，本来终止当前会话是可以的]
 */

main();
function main() {
    if (commIsStandalone(db)) return;

    // 3.a
    var contextList = null;
    try {
        contextList = db.list(SDB_LIST_CONTEXTS).toArray();
    } catch (e) {
        throw buildException("TODO 3.a: db.list(SDB_LIST_CONTEXTS) fail", e);
    }
    var context = JSON.parse(contextList[Math.ceil(Math.random() * 1000) % contextList.length]);
    var SessionID = context.SessionID;
    var NodeID = new InfoByNodeName(context.NodeName).getNodeIdByNodeName();
    try {
        db.forceSession(SessionID, {NodeID: NodeID});
    } catch (e) {
        println(JSON.stringify(context));
        if( isSystemEDU( context ) && e === -264 )
        {
        }
        else
        {
           throw buildException("TODO 3.a: db.forceSession(SessionID, {NodeID: NodeID}) fail", e);
        }
    }
    sleep(500);
    var temp = null;
    try {
        temp = db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray();
    } catch (e) {
        println(JSON.stringify(context));
        throw buildException("TODO 3.a: db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray() fail", e);
    }
    if (temp.length === 0) {
        println("3.a 查询到数组长度为：" + temp.length);
        println(temp);
        println(JSON.stringify(context));
        throw buildException("inner current sesssion be forced", null);
    }

    // 3.d
    var contextList = null;
    try {
        contextList = db.list(SDB_LIST_CONTEXTS).toArray();
    } catch (e) {
        throw buildException("TODO 3.d: db.list(SDB_LIST_CONTEXTS) fail", e);
    }
    var context = JSON.parse(contextList[Math.ceil(Math.random() * 1000) % contextList.length]);
    var SessionID = context.SessionID;
    var NodeID = new InfoByNodeName(context.NodeName).getNodeIdByNodeName();
    var HostName = context.NodeName.split(":")[0];
    try {
        temp = db.forceSession(SessionID, {NodeID: NodeID, HostName: HostName});
    } catch (e) {
        println(JSON.stringify(context));
        if( isSystemEDU( context ) && e === -264 )
        {
        }
        else
        {
           throw buildException("TODO 3.d: db.forceSession(SessionID, {NodeID: NodeID}) fail", e);
        }
    }
    sleep(500);
    var temp = null;
    try {
        temp = db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray();
    } catch (e) {
        println(JSON.stringify(context));
        throw buildException("TODO 3.d: db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray() fail", e);
    }
    if (temp.length === 0) {
        println("3.d 查询到数组长度为：" + temp.length);
        println(temp);
        println(JSON.stringify(context));
        throw buildException("inner current sesssion be forced", null);
    }

    // 3.e
    var contextList = null;
    try {
        contextList = db.list(SDB_LIST_CONTEXTS).toArray();
    } catch (e) {
        throw buildException("TODO 3.d: db.list(SDB_LIST_CONTEXTS) fail", e);
    }
    var context = JSON.parse(contextList[Math.ceil(Math.random() * 1000) % contextList.length]);
    var SessionID = context.SessionID;
    var GroupName = new InfoByNodeName(context.NodeName).getGroupNameByNodeName();
    var HostName = context.NodeName.split(":")[0];
    var svcname = context.NodeName.split(":")[1];
    try {
        db.forceSession(SessionID, {GroupName: GroupName, HostName: HostName, svcname: svcname});
    } catch (e) {
        println(JSON.stringify(context));
        if( isSystemEDU( context ) && e === -264 )
        {
        }
        else
        { 
           throw buildException("TODO 3.e: db.forceSession(SessionID, {NodeID: NodeID}) fail", e);
        }
    }
    sleep(500);
    var temp = null;
    try {
        temp = db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray();
    } catch (e) {
        println(JSON.stringify(context));
        throw buildException("TODO 3.e: db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray() fail", e);
    }
    if (temp.length === 0) {
        println("3.e 查询到数组长度为：" + temp.length);
        println(temp);
        println(JSON.stringify(context));
        throw buildException("inner current sesssion be forced", null);
    }

    // 3.b
    var contextList = null;
    try {
        contextList = db.list(SDB_LIST_CONTEXTS).toArray();
    } catch (e) {
        throw buildException("TODO 3.b: db.list(SDB_LIST_CONTEXTS) fail", e);
    }
    var context = JSON.parse(contextList[Math.ceil(Math.random() * 1000) % contextList.length]);
    var SessionID = context.SessionID;
    var HostName = context.NodeName.split(":")[0];
    try {
        db.forceSession(SessionID, {HostName: HostName});
    } catch (e) {
        if (e !== -264) {
            println(JSON.stringify(context));
            throw buildException("TODO 3.b: db.forceSession(SessionID, {NodeID: NodeID}) fail", e);
        }
    }
    sleep(500);
    var temp = null;
    try {
        temp = db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray();
    } catch (e) {
        println(JSON.stringify(context));
        throw buildException("TODO 3.b: db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray() fail", e);
    }
    if (temp.length === 0) {
        println("3.b 查询到数组长度为：" + temp.length);
        println(temp);
        println(JSON.stringify(context));
        throw buildException("inner current sesssion be forced", null);
    }

    // println("3.c==============");
    // 3.d
    var contextList = null;
    try {
        contextList = db.list(SDB_LIST_CONTEXTS).toArray();
    } catch (e) {
        throw buildException("TODO 3.d: db.list(SDB_LIST_CONTEXTS) fail", e);
    }
    var context = JSON.parse(contextList[Math.ceil(Math.random() * 1000) % contextList.length]);
    var SessionID = context.SessionID;
    var svcname = context.NodeName.split(":")[1];
    try {
        db.forceSession(SessionID, {svcname: svcname});
    } catch (e) {
        if (e !== -264) {
            println(JSON.stringify(context));
            throw buildException("TODO 3.b: db.forceSession(SessionID, {NodeID: NodeID}) fail", e);
        }
    }
    sleep(500);
    var temp = null;
    try {
        temp = db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray();
    } catch (e) {
        println(JSON.stringify(context));
        throw buildException("TODO 3.d: db.list(SDB_LIST_CONTEXTS, {SessionID: SessionID, NodeName: context.NodeName}).toArray() fail", e);
    }
    if (temp.length === 0) {
        println("3.d 查询到数组长度为：" + temp.length);
        println(temp);
        println(JSON.stringify(context));
        throw buildException("inner current sesssion be forced", null);
    }
}


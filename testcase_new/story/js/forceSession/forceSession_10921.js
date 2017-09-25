/**************************************
 * @Description: 测试用例 seqDB-10921 :: 版本: 1 :: 指定sessionID为系统EDU
 * @author: ouyangzhongnan
 * @Date: 2017-01-05
 * @RunDemo:
 * sdb -f "func.js,forceSession_10921.js" -e "var CHANGEDPREFIX='PREFIX';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/
/*
 步骤：
 1、连接coord节点
 2、查看集群session信息（如db.list(2, {Global:true})查看，“Type”为非Agent的会话为系统EDU）
 3、执行db.forceSession(sessionID,options)终止会话，其中sessionID为系统EDU（如指定session为1）
 4、查看指定session是否存在
 结果：
 1、操作失败，返回对应错误信息，查看指定session还存在
 */
// 备注：该用例得并行跑

main();
function main() {
    if (commIsStandalone(db)) return;

    var sessionID = null; // 要被force的session的id
    var forceBefore = null;
    var forceAfter = null;

    // 获取所有系统EDU类型的session，并随机从中取得一个用于force
    try 
    {
        var sessionList = db.list(2, {
            Global: true,
            Status: {$ne: "Waiting"},
            Type: {$nin: ["Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent"]}
        }).toArray();
    }
    catch (e) 
    {
        throw buildException("listSession", e, "listSession by session=2 options=" + JSON.stringify({
                Global: true,
                Type: {$nin: ["Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent"]}
            }));
    }
    var randomNum = Math.ceil(Math.random() * 10000) % sessionList.length;
    var session = JSON.parse(sessionList[randomNum]);
    sessionID = session.SessionID;

    // 加 Status: {$ne:"Waiting"}, Type:{$nin:["Agent","ShardAgent","CoordAgent","ReplAgent","HTTPAgent"]} 因为通过sessionid有可能也能查到非系统EDU类型的session
    try 
    {
        forceBefore = db.list(2, {
            Global: true,
            SessionID: sessionID,
            Status: {$ne: "Waiting"},
            Type: {$nin: ["Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent"]}
        }).toArray();
    }
    catch (e) 
    {
        throw buildException("listSession", e, "forceBefore:listSession by session=2 options=" + JSON.stringify({
                Global: true,
                SessionID: sessionID
            }));
    }


    // force 一个系统EDU类型的session
    try 
    {
        db.forceSession(sessionID, {Global: true});
    }
    catch (e) 
    {
        if (e != -264) {
            throw buildException("forceSession", e, "forceSession by SessionID(session type is system EDU) and set options is Global=true", "forceSession success and throw exception(-264)", "forceSession throw exception and exception is not equals -264");
        }
    }

    // 检查forc的系统session是否都还在
    try
    {
        forceAfter = db.list(2, {
            Global: true,
            SessionID: sessionID,
            Status: {$ne: "Waiting"},
            Type: {$nin: ["Agent", "ShardAgent", "CoordAgent", "ReplAgent", "HTTPAgent"]}
        }).toArray();
    }
    catch (e) 
    {
        throw buildException("listSession", e, "forceAfter:listSession by session=2 options=" + JSON.stringify({
                Global: true,
                SessionID: sessionID
            }));
    } 
}

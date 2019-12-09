/**************************************
 * @Description: 测试用例 seqDB-10923 :: 版本: 1 :: options参数非法校验
 * @author: ouyangzhongnan
 * @Date: 2017-01-05
 * @RunDemo:
 * sdb -f "func.js,commlib.js,forceSession_10923.js" -e "var CHANGEDPREFIX='PREFIX';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/
/*
 步骤：
 1、连接coord节点
 2、执行db.forceSession(sessionID,options)终止会话，其中options参数设置非法，分别验证如下情况：
 a、参数非法，如db.forceSession(31,{Host:"test"})
 b、options参数为空，如db.forceSession(31,{})
 c、options参数格式非法，如db.forceSession(31,{1001})
 d、nodeID类型非法，如非int类型
 4、查看操作结果
 结果:
 1、操作失败，返回对应错误信息
 */

main();
function main ()
{
    if( commIsStandalone( db ) ) return;

    // var conn = getConn(COORDHOSTNAME + ":" + COORDSVCNAME);
    var conn = db;

    var currentSession = conn.list( SDB_LIST_SESSIONS_CURRENT, { Global: false } ).next().toObj();

    // TODO 2.a
    try
    {
        conn.forceSession( currentSession.SessionID, { Host: "testtest" } );
        // throw new Error(0); 这些都先注释起来，之前因为之前许导说sessionID存在就直接force掉，忽略后面的参数不校验
    } catch( e )
    {
        println( e );
        if( e == "Error: 0" )
        {
            throw buildException( "TODO 2.a not validate the options {Host: \"testtest\"}", e );
        } else
        {
            println( e );
            throw buildException( "TODO 2.a conn.forceSession(currentSession.SessionID, {Host: \"testtest\"}) fail", e );
        }
    }

    // TODO 2.b
    var currentSession = conn.list( SDB_LIST_SESSIONS_CURRENT, { Global: false } ).next().toObj();
    try
    {
        conn.forceSession( currentSession.SessionID, {} );
        // throw new Error(0);
    }
    catch( e ) 
    {
        if( e == "Error: 0" )
        {
            throw buildException( "TODO 2.b not validate the options {}", e );
        } else
        {
            println( e );
            throw buildException( "TODO 2.b conn.forceSession(currentSession.SessionID, {}) fail", e );
        }
    }

    // // TODO 2.c (这种情况js脚本并不能跑起来，本事语法上就有错误，所以不需要验证)
    // // try
    // // {
    // // 	db.forceSession( currentSession.SessionID, {1001} );
    // // }
    // // catch(e)
    // // {
    // // 	println(e);
    // // }

    // TODO 2.d
    var currentSession = conn.list( SDB_LIST_SESSIONS_CURRENT, { Global: false } ).next().toObj();
    try
    {
        conn.forceSession( currentSession.SessionID, { NodeID: "abcd" } );
        throw new Error( -6 );
    }
    catch( e )
    {
        if( e !== -6 )
        {
            throw buildException( "TODO 2.d not validate the options {NodeID: \"abcd\"}", e );
        }
    }
}
/**************************************
 * @Description: 测试用例 seqDB-10924 :: 版本: 1 :: options参数非法校验
 * @author: ouyangzhongnan
 * @Date: 2017-01-05
 * @RunDemo:
 * sdb -f "func.js,forceSession_10924.js" -e "var CHANGEDPREFIX='PREFIX';var COORDHOSTNAME='sdbserver01';var COORDSVCNAME='11810'"
 **************************************/

/*
 步骤：
 1、连接coord节点
 2、执行db.forceSession(sessionID,options)终止会话，其中sessionID参数设置非法，分别验证如下情况：
 a、为空，如db.forceSession(options)
 b、类型非法（非int类型），如db.forceSession(“123”,options)
 4、查看操作结果
 结果：
 1、操作失败，返回对应错误信息
 */

main();
function main() {
    if (commIsStandalone(db)) return;

    // TODO 2.a
    var error = "Error: Sdb.forceSession(): wrong arguments";
    try 
    {
        db.forceSession();
    } 
    catch (e) 
    {       
        if (String(e) !== error) 
        {
            throw buildException("forceSession", new Error(), "forceSession is null", error, e);
        }
    }

    // TODO 2.b 因为sessionid传入字符串，不报错，这里类型非法采用其他类型验证
    try 
    {
        db.forceSession(true);
    } 
    catch (e) 
    {
        if (String(e) !== error)
        {   
            throw buildException("forceSession", new Error(), "forceSession but sessionid type is error", error, e);
        }
    }
}
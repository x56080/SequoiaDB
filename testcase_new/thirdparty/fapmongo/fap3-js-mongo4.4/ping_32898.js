/******************************************************************************
 * @Description   : seqDB-32898:ping检查连接状态
 *    seqDB-32905:find带$query操作符
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   // seqDB-32898: { "ping": 1 }
   // 正常连接，检查连接状态
   var rc = db.runCommand( { "ping": 1 } );
   assert.eq( rc, { "ok": 1 } );
   // ping失败的场景需要构造服务异常（如：coord异常导致fapmongo连接异常），暂不实现自动化


   // seqDB-32905 带$query操作符（JS 实际没有$query操作符，只能以如下方式简单覆盖。重点在 JAVA 驱动覆盖测试）
   var rc = db.runCommand( { "$query": { "ping": 1 } } );
   assert.eq( rc, { "ok": 1 } );
}
/******************************************************************************
 * @Description   : seqDB-32899:getnonce获取身份验证随机数
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

main();
function main ()
{
   var rc = db.runCommand( { "getnonce": 1 } );
   // 对接mongo引擎，返回结果：{ "nonce" : "5f12e33916137d8d", "ok" : 1 }
   // 说明：fapmongo只为了对接getnonce执行不报错，且原生mongo新版本getnonce已废弃，fapmongo不需要关注nonce随机数正确性
   assert.eq( rc, { "nonce": "0", "ok": 1 } );
}
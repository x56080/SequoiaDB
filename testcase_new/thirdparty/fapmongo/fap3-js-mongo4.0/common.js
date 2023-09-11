/******************************************************************************
 * @Description   : mongo js 公共方法
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2023.08.16
 * @LastEditTime  : 2023.08.16
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/

function checkResults ( cursor, expDocs )
{
   var docs = new Array();
   while( cursor.hasNext() )
   {
      var doc = cursor.next();
      docs.push( doc );
   }
   cursor.close();
   assert.eq( JSON.stringify( docs ), expDocs );
}

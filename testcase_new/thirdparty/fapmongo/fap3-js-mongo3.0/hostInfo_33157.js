/******************************************** 
 * @Author        : huangxiaoni 
 * @CreateTime    : 2023-09-01
 * @LastEditors   : huangxiaoni 
 * @LastEditTime  : 2023-09-01
 * @Description   : seqDB-33157:hostInfo
*********************************************/

main();
function main ()
{
   // db.hostInfo
   var rc = db.hostInfo();
   checkResults( rc );

   // db.runCommand( { "hostInfo": 1 } )
   var rc = db.runCommand( { "hostInfo": 1 } );
   checkResults( rc );
}

function checkResults ( rc )
{
   assert.eq( rc.ok, 1 );
   var expFields = ["system", "os", "extra", "ok"];
   for( var i = 0; i < expFields.length; i++ )
   {
      assert.eq( rc.hasOwnProperty( expFields[i] ), true );
   }
}
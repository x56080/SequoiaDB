/******************************************************************************
 * @Description   :seqDB-31206:dropCS接口参数Comment校验
 *                 seqDB-31207:dropCL接口参数Comment校验
 *                 seqDB-31208:truncate接口参数Comment校验
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.21
 * @LastEditTime  : 2023.04.25
 * @LastEditors   : Bi Qin
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName1 = "cs_31206_31207_31208_1";
   var csName2 = "cs_31206_31207_31208_2";
   var clName1 = "cl_31207";
   var clName2 = "cl_31208";
   var comment = generateRandomString( 20 ) + "测试用例";

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_31206_31207_31208_" );

   commCreateCS( db, csName1 );
   var dbcs2 = commCreateCS( db, csName2 );
   var dbcl1 = commCreateCL( db, csName2, clName1 );
   var dbcl2 = commCreateCL( db, csName2, clName2 );

   insertBulkData( dbcl1, 1000 );
   insertBulkData( dbcl2, 2000 );
   //无效值校验
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.dropCS( csName1, { Comment: 1 } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcs2.dropCL( clName1, { Comment: 1 } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl2.truncate( { Comment: 1 } );
   } );

   var array = new Array( 35000 );
   array = array.join( "abcd" );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.dropCS( csName1, { Comment: array } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcs2.dropCL( clName1, { Comment: array } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl2.truncate( { Comment: array } );
   } );

   //有效值检验
   db.dropCS( csName1, { Comment: comment } );
   var cursor = db.list( SDB_LIST_RECYCLEBIN, { OriginName: csName1 } );
   checkCursorComment( cursor, comment );

   dbcs2.dropCL( clName1, { Comment: comment } );
   cursor = db.list( SDB_LIST_RECYCLEBIN, { OriginName: csName2 + "." + clName1 } );
   checkCursorComment( cursor, comment );
   cursor = db.snapshot( SDB_SNAP_RECYCLEBIN, { OriginName: csName2 + "." + clName1 } );
   checkCursorComment( cursor, comment );

   dbcl2.truncate( { Comment: comment } )
   cursor = db.getRecycleBin().list( { OriginName: csName2 + "." + clName2 } );
   checkCursorComment( cursor, comment );
   cursor = db.getRecycleBin().snapshot( { OriginName: csName2 + "." + clName2 } );
   checkCursorComment( cursor, comment );

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_31206_31207_31208_" );
}

function generateRandomString ( length )
{
   var chars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+~`|}{[]\:;?><,./-=';
   var randomString = '';
   for( var i = 0; i < length; i++ )
   {
      var randomNum = Math.floor( Math.random() * chars.length );
      randomString += chars.substring( randomNum, randomNum + 1 );
   }
   return randomString;
}
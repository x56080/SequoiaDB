/******************************************************************************
 * @Description   : seqDB-24908:detectdisk参数测试
 * @Author        : liuli
 * @CreateTime    : 2022.01.07
 * @LastEditTime  : 2022.01.07
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );
function test ()
{
   try
   {
      // 校验默认配置
      var actConfig = { detectdisk: "TRUE" };
      checkSnapshot( db, actConfig );

      var config = { detectdisk: false };
      db.updateConf( config );
      var actConfig = { detectdisk: "FALSE" };
      checkSnapshot( db, actConfig );

      var config = { detectdisk: true };
      db.updateConf( config );
      var actConfig = { detectdisk: "TRUE" };
      checkSnapshot( db, actConfig );

      var config = { detectdisk: 1 };
      db.updateConf( config );
      checkSnapshot( db, actConfig );

      var actConfig = { detectdisk: "FALSE" };
      var config = { detectdisk: 0 };
      db.updateConf( config );
      checkSnapshot( db, actConfig );

      var config = { detectdisk: -1 };
      db.updateConf( config );
      checkSnapshot( db, actConfig );

      var config = { detectdisk: "true" };
      db.updateConf( config );
      var actConfig = { detectdisk: "TRUE" };
      checkSnapshot( db, actConfig );

      var config = { detectdisk: "aaa" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, actConfig );
   }
   finally
   {
      db.deleteConf( { detectdisk: "" } );
   }
}

function checkSnapshot ( sdb, option )
{
   var cursor = sdb.snapshot( SDB_SNAP_CONFIGS, {}, { detectdisk: "" } );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in option )
      {
         assert.equal( obj[key], option[key] );
      }
   }
   cursor.close();
}
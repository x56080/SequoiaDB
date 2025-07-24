/******************************************************************************
 * @Description   : seqDB-26832:updateConf更新非官方参数
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.08.10
 * @LastEditTime  : 2022.08.16
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var groupName = commGetDataGroupNames( db )[0];
   var cond = { "GroupName": groupName };
   try
   {
      // 官方参数校验
      // 1)验证Force默认值
      var config = { preferredinstance: "A" };
      var expConfig = { preferredinstance: "A" };
      var option = { "GroupName": groupName };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 2)验证Force指定为数字
      var config = { preferredinstance: "S" };
      var expConfig = { preferredinstance: "S" };
      var option = { "GroupName": groupName, "Force": 1 };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 3)验证Force指定为字符串
      var config = { preferredinstance: "A" };
      var expConfig = { preferredinstance: "A" };
      var option = { "GroupName": groupName, "Force": "true" };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 4)验证Force指定为空字符串
      var config = { preferredinstance: "S" };
      var expConfig = { preferredinstance: "S" };
      var option = { "GroupName": groupName, "Force": "" };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 5)验证Force指定为false
      var config = { preferredinstance: "A" };
      var expConfig = { preferredinstance: "A" };
      var option = { "GroupName": groupName, "Force": false };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 6)验证Force指定为true
      var config = { preferredinstance: "S" };
      var expConfig = { preferredinstance: "S" };
      var option = { "GroupName": groupName, "Force": true };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );

      // 隐藏参数校验
      // 1)验证Force默认值  
      var config = { extendthreshold: 2 };
      var expConfig = { extendthreshold: 2 };
      var option = { "GroupName": groupName };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 2)验证Force指定为数字
      var config = { extendthreshold: 4 };
      var expConfig = { extendthreshold: 4 };
      var option = { "GroupName": groupName, "Force": 1 };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 3)验证Force指定为字符串
      var config = { extendthreshold: 8 };
      var expConfig = { extendthreshold: 8 };
      var option = { "GroupName": groupName, "Force": "true" };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 4)验证Force指定为空字符串
      var config = { extendthreshold: 16 };
      var expConfig = { extendthreshold: 16 };
      var option = { "GroupName": groupName, "Force": "" };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 5)验证Force指定为false
      var config = { extendthreshold: 64 };
      var expConfig = { extendthreshold: 64 };
      var option = { "GroupName": groupName, "Force": false };
      db.updateConf( config, option );
      checkConfValue( db, cond, expConfig );
      // 6)验证Force指定为true
      var config = { extendthreshold: 128 };
      var expConfig = { extendthreshold: 128 };
      var option = { "GroupName": groupName, "Force": true };
      db.updateConf( config, option )
      checkConfValue( db, cond, expConfig );

      // 非官方参数校验
      // 1.添加当前不存在的非官方参数
      // 1)验证Force默认值
      var config = { abc: 1 };
      var option = { "GroupName": groupName };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNotKey( db, cond, config );
      // 2)验证Force指定为数字
      var config = { efg: 2 };
      var option = { "GroupName": groupName, "Force": 1 };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNotKey( db, cond, config );
      // 3)验证Force指定为字符串
      var config = { ijk: 3 };
      var option = { "GroupName": groupName, "Force": "true" };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNotKey( db, cond, config );
      // 4)验证Force指定为空字符串
      var config = { mno: 5 };
      var option = { "GroupName": groupName, "Force": "" };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNotKey( db, cond, config );
      // 5)验证Force指定为false
      var config = { rst: 7 };
      var option = { "GroupName": groupName, "Force": false };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNotKey( db, cond, config );
      // 6)验证Force指定为true
      var config = { xyz: 9 };
      var option = { "GroupName": groupName, "Force": true };
      db.updateConf( config, option );
      checkConfKey( db, cond, config );

      // 2.更新当前已存在的非官方参数
      // 1)验证Force默认值
      var config = { xyz: 10 };
      var expConfig = { xyz: 10 };
      var option = { "GroupName": groupName };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNoValue( db, cond, expConfig );
      // 2)验证Force指定为数字
      var config = { xyz: 12 };
      var expConfig = { xyz: 12 };
      var option = { "GroupName": groupName, "Force": 1 };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNoValue( db, cond, expConfig );
      // 3)验证Force指定为字符串
      var config = { xyz: 14 };
      var expConfig = { xyz: 14 };
      var option = { "GroupName": groupName, "Force": "true" };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNoValue( db, cond, expConfig );
      // 4)验证Force指定为空字符串
      var config = { xyz: 16 };
      var expConfig = { xyz: 16 };
      var option = { "GroupName": groupName, "Force": "" };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNoValue( db, cond, expConfig );
      // 5)验证Force指定为false
      var config = { xyz: 18 };
      var expConfig = { xyz: 18 };
      var option = { "GroupName": groupName, "Force": false };
      assert.tryThrow( SDB_INVALIDARG, function() 
      {
         db.updateConf( config, option );
      } );
      checkConfNoValue( db, cond, expConfig );
      // 6)验证Force指定为true
      var config = { xyz: 20 };
      var expConfig = { xyz: 20 };
      var option = { "GroupName": groupName, "Force": true };
      db.updateConf( config, option )
      checkConfValue( db, cond, expConfig );
   } finally
   {
      db.deleteConf( { preferredinstance: "", extendthreshold: "", xyz: "" } );
      var config = { xyz: 20 };
      checkConfNotKey( db, cond, config );
   }
}

function checkConfValue ( db, cond, expConfig )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, cond );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in expConfig )
      {
         assert.equal( obj[key], expConfig[key], "预期value值相等" );
      }
   }
   cursor.close();
}

function checkConfNoValue ( db, cond, expConfig )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, cond );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      print( JSON.stringify( obj ) );
      for( var key in expConfig )
      {
         assert.notEqual( obj[key], expConfig[key], "预期value值不相等" );
      }
   }
   cursor.close();
}

function checkConfKey ( db, cond, config )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, cond );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var keys = Object.keys( obj );
      var key = Object.keys( config )[0];
      assert.notEqual( keys.indexOf( key ), -1, "预期key在keys中" );
   }
   cursor.close();
}

function checkConfNotKey ( db, cond, config )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, cond );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var keys = Object.keys( obj );
      var key = Object.keys( config )[0];
      assert.equal( keys.indexOf( key ), -1, "预期key不在keys中" );
   }
   cursor.close();
}
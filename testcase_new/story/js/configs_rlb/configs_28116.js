/******************************************************************************
 * @Description   : seqDB-28116:deleteConf忽略大小写
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.10.04
 * @LastEditTime  : 2022.10.12
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var groupName = commGetDataGroupNames( db )[0];
   var node = db.getRG( groupName ).getSlave();
   var option = { "HostName": node.getHostName(), "ServiceName": node.getServiceName() };
   var isSuccess = false;

   try
   {
      // preferredperiod参数校验
      // 部分大写
      var isSuccess = false;
      var config1 = { Preferredperiod: 90 };
      var expConfig1 = { preferredperiod: 90 };
      var config2 = { Preferredperiod: 1 };
      var expConfig2 = { preferredperiod: 60 };
      effectiveOnline( db, option, config1, expConfig1, config2, expConfig2 );
      // 全部大写
      config1 = { PREFERREDPERIOD: 120 };
      expConfig1 = { preferredperiod: 120 };
      config2 = { PREFERREDPERIOD: 1 };
      effectiveOnline( db, option, config1, expConfig1, config2, expConfig2 );

      // maxprefpool参数校验
      // 部分大写
      config1 = { Maxprefpool: 200 };
      expConfig1 = { maxprefpool: 200 };
      config2 = { Maxprefpool: 1 };
      expConfig2 = { maxprefpool: 0 };
      effectiveRestart( db, option, config1, expConfig1, config2, expConfig2, node );
      // 全部大写
      config1 = { MAXPREFPOOL: 300 };
      expConfig1 = { maxprefpool: 300 };
      config2 = { MAXPREFPOOL: 1 };
      effectiveRestart( db, option, config1, expConfig1, config2, expConfig2, node );
      isSuccess = true;
   }
   finally
   {
      if( !isSuccess )
      {
         var configs = { preferredperiod: 1, maxprefpool: 1 };
         try
         {
            db.deleteConf( configs, options );
         }
         catch( e )
         {
            println( "---deleteConf error is " + e.message );
         }
         node.stop();
         node.start();
         commCheckBusinessStatus( db );
      }
   }
}

function effectiveOnline ( db, cond, config1, expConfig1, config2, expConfig2 )
{
   db.updateConf( config1, cond );
   checkSnapshot( db, cond, config1, expConfig1 );
   db.deleteConf( config2, cond );
   checkSnapshot( db, cond, config2, expConfig2 );
}

function effectiveRestart ( db, cond, config1, expConfig1, config2, expConfig2, node )
{
   updateConf( db, config1, cond, SDB_RTN_CONF_NOT_TAKE_EFFECT );
   node.stop();
   node.start();
   commCheckBusinessStatus( db );
   checkSnapshot( db, cond, config1, expConfig1 );
   deleteConf( db, config2, cond, SDB_RTN_CONF_NOT_TAKE_EFFECT );
   node.stop();
   node.start();
   commCheckBusinessStatus( db );
   checkSnapshot( db, cond, config2, expConfig2 );
}

function checkSnapshot ( db, cond, config, expConfig )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, cond );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var keys = Object.keys( obj );
      var key = Object.keys( config )[0];
      assert.equal( keys.indexOf( key ), -1, "预期key不在keys中" );
      for( var key in expConfig )
      {
         assert.equal( obj[key], expConfig[key], "预期value值相等" );
      }
   }
   cursor.close();
}
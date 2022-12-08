/******************************************************************************
 * @Description   : seqDB-26806:updateConf忽略大小写
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.08.04
 * @LastEditTime  : 2022.08.09
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var groupName = commGetDataGroupNames( db )[0];
   var option = { "GroupName": groupName };

   try
   {
      // transisolation参数校验
      var config = { TransIsolation: 1 };
      var expConfig = { transisolation: 1 };
      db.updateConf( config, option );
      checkSnapshot( db, config, option, expConfig );

      var config = { TRANSISOLATION: 2 };
      var expConfig = { transisolation: 2 };
      db.updateConf( config, option );
      checkSnapshot( db, config, option, expConfig );

      // preferredinstance参数校验
      var config = { PreferredInstance: "A" };
      var expConfig = { preferredinstance: "A" };
      db.updateConf( config, option );
      checkSnapshot( db, config, option, expConfig );

      var config = { PREFERREDINSTANCE: "S" };
      var expConfig = { preferredinstance: "S" };
      db.updateConf( config, option );
      checkSnapshot( db, config, option, expConfig );

      // directioinlob参数校验
      var config = { DirectIoInLob: "true" };
      var expConfig = { directioinlob: "TRUE" };
      db.updateConf( config, option );
      checkSnapshot( db, config, option, expConfig );

      var config = { DIRECTIOINLOB: "false" };
      var expConfig = { directioinlob: "FALSE" };
      db.updateConf( config, option );
      checkSnapshot( db, config, option, expConfig );

      // maxprefpool参数校验
      var config = { MaxprefPool: 200 };
      var expConfig = { maxprefpool: 200 };
      updateConf( db, config, option, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
      checkSnapshot( db, config, option, expConfig );

      var config = { MAXPREFPOOL: 300 };
      var expConfig = { maxprefpool: 300 };
      updateConf( db, config, option, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
      checkSnapshot( db, config, option, expConfig );

      // archiveon参数校验
      var config = { ArchiveOn: "true" };
      var expConfig = { archiveon: "TRUE" };
      updateConf( db, config, option, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
      checkSnapshot( db, config, option, expConfig );

      var config = { ARCHIVEON: "false" };
      var expConfig = { archiveon: "FALSE" };
      updateConf( db, config, option, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
      checkSnapshot( db, config, option, expConfig );

      // transactionon参数校验
      var config = { TransActionOn: "false" };
      var expConfig = { transactionon: "FALSE" };
      updateConf( db, config, option, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
      checkSnapshot( db, config, option, expConfig );

      var config = { TRANSACTIONON: "true" };
      var expConfig = { transactionon: "TRUE" };
      updateConf( db, config, option, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
      checkSnapshot( db, config, option, expConfig );
   } finally
   {
      var configs = {
         transisolation: "",
         preferredinstance: "",
         directioinlob: "",
         maxprefpool: "",
         archiveon: "",
         transctionon: ""
      };
      deleteConf( db, configs, option, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.getRG( groupName ).stop();
      db.getRG( groupName ).start();
      commCheckBusinessStatus( db );
   }
}

function checkSnapshot ( db, config, cond, expConfig )
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

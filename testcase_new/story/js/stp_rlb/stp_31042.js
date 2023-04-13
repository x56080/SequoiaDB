/******************************************************************************
 * @Description   : seqDB-31042:maxtimemapsize参数校验
 * @Author        : liuli
 * @CreateTime    : 2023.04.10
 * @LastEditTime  : 2023.04.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var stp = new Stp( STPHOSTNAME, STPSVCNAME );
   try
   {
      // maxtimemapsize校验合法参数
      var config = { maxtimemapsize: -1 };
      var expConfig = { maxtimemapsize: -1 };
      stp.updateConf( config );
      checkSnapshot( stp, expConfig );

      var config = { maxtimemapsize: 5000 };
      var expConfig = { maxtimemapsize: 5000 };
      stp.updateConf( config );
      checkSnapshot( stp, expConfig );

      var config = { maxtimemapsize: 0 };
      var expConfig = { maxtimemapsize: 0 };
      stp.updateConf( config );
      checkSnapshot( stp, expConfig );

      var timeMap = stp.getTimeMap().toObj();
      assert.equal( timeMap, { "TimeMap": [] } );

      var config = { maxtimemapsize: "" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         stp.updateConf( config );
      } );
      checkSnapshot( stp, expConfig );

      var config = { maxtimemapsize: "aaa" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         stp.updateConf( config );
      } );
      checkSnapshot( stp, expConfig );

      var config = { maxtimemapsize: "1" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         stp.updateConf( config );
      } );
      checkSnapshot( stp, expConfig );

      var config = { maxtimemapsize: true };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         stp.updateConf( config );
      } );
      checkSnapshot( stp, expConfig );
   }
   finally
   {
      stp.updateConf( { maxtimemapsize: 525600 } );
      stp.close();
   }
}

function checkSnapshot ( stp, option )
{
   var obj = stp.getConf().toObj();
   for( var key in option )
   {
      assert.equal( obj[key], option[key] );
   }
}
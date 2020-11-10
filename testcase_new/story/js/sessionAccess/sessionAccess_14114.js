/* *****************************************************************************
@description: seqDB-14114:设置timeout值和session值，查询记录超时
@author: 2018-1-29 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

testConf.clName = CHANGEDPREFIX + "_14114";
testConf.clOpt = { ReplSize: 0 };
//SEQUOIADBMAINSTREAM-5245
//main( test );

function test ( testPara )
{
   bulkInsert( testPara.testCL, 80000 );

   var timeoutValues = [1, 1000, 2000];
   for( var i = 0; i < timeoutValues.length; i++ )
   {
      db.setSessionAttr( { PreferedInstance: "M", Timeout: timeoutValues[i] } );
      try
      {
         testPara.testCL.update( { $set: { a: "aaaaaa" } } );
         throw new Error( "NEED_TIMEOUT_ERROR" );
      }
      catch( e )
      {
         //TODO:这里需确认更新超时不报错和更新几十秒后报错-116是否合理
         if( e.message !== "-13" && e.message !== "-116" )
         {
            throw e;
         }
      }
      finally
      {
         checkTimeoutValue( timeoutValues[i] );
         db.setSessionAttr( { Timeout: -1 } );
      }
   }
}

function checkTimeoutValue ( timeoutValue )
{
   var timeout = db.getSessionAttr().current().toObj().Timeout;
   if( timeout !== timeoutValue )
   {
      throw new Error( "The expected timeout value is " + timeoutValue + ", but the actual timeout value is " + timeout );
   }
}

function bulkInsert ( cl, insertNums )
{
   var batchNums = 10000;
   var recs = [];
   var times = insertNums / batchNums;

   for( var k = 0; k < times; k++ )
   {
      var doc = [];
      for( var i = 0; i < batchNums; ++i )
      {
         doc.push( { a: "string" } );
      }
      cl.insert( doc );
   }
}


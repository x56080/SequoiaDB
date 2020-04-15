/* *****************************************************************************
@description: seqDB-14114:设置timeout值和session值，查询记录超时
@author: 2020-4-15 zhaoxiaoni  Init
***************************************************************************** */
//main();SEQUOIADBMAINSTREAM-5245
function main()
{
   if( commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var clName = CHANGEDPREFIX + "_14114";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCLByOption( db, COMMCSNAME, clName );
   insertData( cl, 80000 );
 
   var timeoutValues = [1, 1000, 2000];
   for( var i = 0; i < timeoutValues.length; i++ )
   {
      db.setSessionAttr({ PreferedInstance: "M", Timeout: timeoutValues[i] });//资料需补充Timeout取值范围，取值为1时，结果为非预期
      try
      {
         cl.update({ $set: {a: "aaaaaa" }});
         throw "NEED_TIMEOUT_ERROR";
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
         db.setSessionAttr({ Timeout: -1});
      }
   }

   commDropCL( db, COMMCSNAME, clName, false, false );
}

function checkTimeoutValue( timeoutValue )
{
   try
   {
      var timeout = db.getSessionAttr().toObj().Timeout;
      if ( timeout !== timeoutValue)
      {
         throw "The expected timeout value is " + timeoutValue + ", but the actual timeout value is " + timeout;
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
}

function bulkInsert( cl, insertNums )
{
   var batchNums = 10000;
   var recs = [];
   var times = insertNums/batchNums;

   for(var k = 0; k < times; k++)
   {
      var doc = [];
      for( var i = 0; i < batchNums; ++i )
      {
         doc.push({ a: "string"});
      }
      cl.insert( doc );
   }
}


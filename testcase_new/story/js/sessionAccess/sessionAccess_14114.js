/* *****************************************************************************
@discretion: setSessionAttr(),set instatceid and timeout, query timeout;test getSessionAttr(); 
@author��2018-1-29 wuyan  Init
***************************************************************************** */
main();
function main ()
{
   try
   {
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      if( true == commIsStandalone( sdb ) )
      {
         println( "run mode is standalone" );
         return;
      }

      //create cl and insert data
      var csName = CHANGEDPREFIX + "_cs14114";
      var clName = CHANGEDPREFIX + "_sessionAcess14114";
      commCreateCS( sdb, csName, false, "Failed to create CS." );
      var options = { ReplSize: 0 };
      var dbcl = commCreateCL( sdb, csName, clName, options, true, true );
      buckInsertData( dbcl, 80000 );

      println( "---begin to test query timeout " );
      testQueryTimeout( csName, clName );

      commDropCS( sdb, csName, false, "Failed to drop CS." );
   }
   catch( e )
   {
      throw buildException( "test session14114", e );
   }
   finally
   {
      if( db != null )
      {
         sdb.close()
      }
   }
}

function testQueryTimeout ( csName, clName )
{
   try
   {
      var timeOutValue = 1000;
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      sdb.setSessionAttr( { PreferedInstance: "M", Timeout: timeOutValue } );
      var dbcl = sdb.getCS( csName ).getCL( clName );
      var rc = dbcl.find().sort( { a: 1 } );
      while( rc.next() )
      {
         var atcObj = rc.current().toObj();
      }
   }
   catch( e )
   {
      if( e !== -13 )
         throw buildException( "check query timeout", e );
   }
   finally
   {
      checkTimeoutValue( sdb, timeOutValue );
      sdb.setSessionAttr( { Timeout: -1 } );
      if( rc != null )
      {
         rc.close();
      }
      if( sdb != null )
      {
         sdb.close();
      }

   }
}

function checkTimeoutValue ( sdb, timeOutValue )
{
   var sessionResult = sdb.getSessionAttr().toObj();
   var timeout = sessionResult.Timeout;
   if( JSON.stringify( timeout ) !== JSON.stringify( timeOutValue ) )
   {
      throw buildException( "checkTimeoutValue()", e, "getSeesionAttr()", timeOutValue, JSON.stringify( timeout ) );
   }

}

function buckInsertData ( dbcl, insertNums, beginNums )
{
   if( undefined == beginNums ) { beginNums = 0; }
   try
   {
      println( "---begin to buckInsert data." );
      var batchNums = 10000;
      var recs = [];
      var times = insertNums / batchNums;

      for( var k = 0; k < times; k++ )
      {
         var doc = [];
         for( var i = 0; i < batchNums; ++i )
         {
            var count = beginNums++
            var no = count;
            var str = getRandomString( 100 ) + "teststr_" + count;
            var inta = count;
            var fc = count + 0.7898;
            var objs = { "no": no, "str": str, "inta": inta, "fc": fc };
            doc.push( objs );
            recs.push( objs );
         }
         dbcl.insert( doc );
      }
      println( "---end bulkInsert data." )
      return recs;
   }
   catch( e )
   {
      throw buildException( "bulkInsertDate()", e );
   }
}

function getRandomString ( len ) 
{
   var str = "";
   var chars = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
   var maxPos = chars.length;
   for( var i = 0; i < len; i++ )
   {
      str += chars.charAt( Math.floor( Math.random() * maxPos ) );
   }
   return str;
}
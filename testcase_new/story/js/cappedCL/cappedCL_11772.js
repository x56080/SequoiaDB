/************************************
*@Description: 批量插入记录,_id超过整型范围
*@author:      luweikang
*@createdate:  2017.7.11
*@testlinkCase:seqDB-11772
**************************************/

main()

function main ()
{
   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "_11772";
   var optionObj = { Capped: true, Size: 4096, Max: 10000000, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //insertData
   println( "---bulk insert data---" )
   var randomNum = Math.floor( Math.random() * 100 );
   var bigStr = createBigStr( randomNum );
   for( var i = 0; i < 20; i++ )
   {
      bulkInsertData( dbcl, bigStr, 100 );
   }

   //check id
   checkId( dbcl, bigStr, 2000 );

   //clean environment after test  
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
   println( "---end the test---" );
}

function bulkInsertData ( dbcl, bigStr, times )
{
   var doc = new Array();
   for( var i = 0; i < times; i++ )
   {
      var options = { a: bigStr };
      doc.push( options );
   }
   try
   {
      dbcl.insert( doc );
   }
   catch( e )
   {
      throw buildException( "bulkInsertData()", e, "bulk insert data", "insert success", "insert fail:" + e );
   }
}

function checkId ( dbcl, bigStr, recordNum )
{
   try
   {
      //dbcl.insert( { a : 1 } );
      var cursor = dbcl.find( null, { '_id': "" } ).sort( { "_id": -1 } ).limit( 1 );
      var actID = cursor.next().toObj()._id;

      var len = fourByte( 55 + bigStr.length );
      var blank = 33554396 % len;
      var countLen = len * recordNum;
      var one = Math.floor( 33554396 / len );
      var blanks = Math.floor( recordNum / one ) * blank;
      var expID = countLen + blanks - len;

      //println(actID+":"+expID)
      if( actID <= 2147483647 )
      {
         throw "ERR_ID_MIN";
      }
      if( Number( actID ) !== expID )
      {
         throw "ERR_ID_VALUE";
      }
      cursor.close();
   }
   catch( e )
   {
      throw buildException( "checkId()", e, "check Id", "find success", "find fail:" + e );
   }
}

function fourByte ( len )
{
   if( len % 4 !== 0 )
   {
      len = len - len % 4 + 4;
   }
   return len;
}

function createBigStr ( randomNum )
{
   var size = 1024 * ( 1024 + randomNum );
   var str = "";
   for( var i = 0; i < size; i++ )
   {
      str = str + "a";
   }
   return str;
}
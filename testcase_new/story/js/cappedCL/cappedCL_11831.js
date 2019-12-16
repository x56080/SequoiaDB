/************************************
*@Description: 创建固定集合，测试update、delete、truncate接口
*@author:      luweikang
*@createdate:  2018.2.11
*@testlinkCase:seqDB-11831
**************************************/

main()

function main ()
{
   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "11831";
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var cl = commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //insertData
   var doc = buildData();
   cl.insert( doc );

   //update
   var options = { $inc: { No: 1 } };
   updateData( cl, options );
   checkResult( cl, doc );

   //delete
   removeData( cl );
   checkResult( cl, doc );

   //truncate,32M>record
   insertBigData( cl, 32 );
   println( "truncate 32M data" );
   truncateData( cl );
   checkResult( cl, null );
   println( "---truncate success---" );

   //truncate,32M<record<96M
   insertBigData( cl, 64 );
   println( "truncate 64M data" );
   truncateData( cl );
   checkResult( cl, null );
   println( "---truncate success---" );

   //truncate,96M<record
   insertBigData( cl, 108 );
   println( "truncate 108M data" );
   truncateData( cl );
   checkResult( cl, null );
   println( "---truncate success---" );

   //insert data check _id
   insertData( cl );

   //clean environment after test
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
   println( "---end the test---" );
}

function buildData ()
{
   var doc = [{ No: 1, a: 10 }, { No: 2, a: 50 }, { No: 3, a: -1001 },
   { No: 4, a: { $decimal: "123.456" } }, { No: 5, a: 101.02 },
   { No: 6, a: { $numberLong: "9223372036854775807" } }, { No: 7, a: { $numberLong: "-9223372036854775808" } },
   { No: 8, a: { $date: "2017-05-01" } }, { No: 9, a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { No: 10, a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { No: 11, a: { $regex: "^z", $options: "i" } },
   { No: 12, a: null },
   { No: 13, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 14, a: "abc" },
   { No: 15, a: { MinKey: 1 } },
   { No: 16, a: { MaxKey: 1 } },
   { No: 17, a: true }, { No: 18, a: false },
   { No: 19, a: { name: "Jack" } },
   { No: 20, a: [1] },
   { No: 21, a: [3] },
   { No: 22, a: 22 },
   { No: 23, a: [23, 23.3, "23"] },
   { No: 24, a: "" }];
   return doc;
}

function checkResult ( cl, expRec )
{
   if( expRec === null )
   {
      try
      {
         var cursor = cl.find();
         cursor.current();
         throw "ERR_CL_NOTNULL";
      }
      catch( e )
      {
         if( e !== -29 && e !== -31 )
         {
            throw buildException( "checkResult()", e, "truncate cl should be null", "-29 || -31 ", e );
         }
      }
      cursor.close();
   }
   else
   {
      var cursor = cl.find();
      checkRec( cursor, expRec );
   }
}

function updateData ( cl, options )
{
   try
   {
      cl.update( options );
      throw "UPDATE_ERROR";
   }
   catch( e )
   {
      if( e !== -279 )
      {
         throw buildException( "updateData()", e, "update record shuld be error", -279, e );
      }
   }
}

function removeData ( cl )
{
   try
   {
      cl.insert( { a: 'test_delete_last_record' } );
      println( "insert a record in the end" );
      var id = cl.findOne().sort( { _id: -1 } ).current().toObj()._id;
      cl.remove( { _id: id } );
      println( "remove last record success" );
      cl.remove();
      throw "REMOVE_ALL_ERROR";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "removeData()", e, "delete record shuld be error", -6, e );
      }
   }
}


function truncateData ( cl )
{
   try
   {
      cl.truncate();
   }
   catch( e )
   {
      throw buildException( "truncateData()", e, "truncate record shuld be success", "success", "faild" );
   }
}


function insertData ( cl )
{
   var doc1 = [{ a: "abcde" },
   { a: "hjsdh" },
   { a: "jdwji" },
   { a: "qwieu" },
   { a: "niwew" }];
   try
   {
      cl.insert( doc1 );
      for( i = 0; i < 5; i++ )
      {
         var cursor = cl.find().sort( { _id: 1 } ).skip( i ).limit( 1 );
         var actId = cursor.current().toObj()._id;
         var expId = i * 60;
         if( actId !== expId )
         {
            throw buildException( "insertData()", e, "check record id", expId, actId );
         }
      }
   }
   catch( e )
   {
      throw buildException( "insertData()", e, "insert record shuld be success", "success", "faild" );
   }
}

function insertBigData ( cl, size )
{
   var recordnum = size * 102;
   var str = "sdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjks";
   var doc3 = new Array();
   var record = { a: str };
   for( i = 0; i < 10; i++ )
   {

      doc3.push( record );
   }
   for( j = 0; j < recordnum; j++ )
   {
      cl.insert( doc3 );
   }
}









/************************************************************************
*@Description:  seqDB-18927:整数位后n位为0，小数位全为0（如10.00）
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18927_json";
   var jsonFile = tmpFileDir + clName + ".json";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );
   
   println( "\n---data type int32, int64, double, decimal to import json file." );
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 800 );
   var expResult = getExpResult( "int32" );
   checkResult( cl, "int32", expResult );
   var expResult = getExpResult( "int64" );
   checkResult( cl, "int64", expResult );
   var expResult = getExpResult( "double" );
   checkResult( cl, "double", expResult );
   var expResult = getExpResult( "decimal" );
   checkResult( cl, "decimal", expResult );
   
   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate( typeFile )
{
   var file = new File( typeFile );
   var left = "1";
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         file.write( '{ a: ' + left + ' }\n' );
         file.write( '{ a: ' + left + '.' + right + ' }\n' );
      }
   }
}

function getExpResult( dataType )
{
   var expResult = []; 
   var left = "1";
   if( dataType == "int64" )
   {
      executeFor( expResult, {a: 10000000000 } );
      executeFor( expResult, {a: 100000000000 } );
      executeFor( expResult, {a: 1000000000000 } );
      executeFor( expResult, {a: 10000000000000 } );
      executeFor( expResult, {a: 100000000000000 } );
      executeFor( expResult, {a: 1000000000000000 } );
      executeFor( expResult, { a: {"$numberLong":"10000000000000000"} });
      executeFor( expResult, { a: {"$numberLong":"100000000000000000"} });
      executeFor( expResult, { a: {"$numberLong":"1000000000000000000"} });
   }
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         if( dataType == "decimal" )
         {
            if( i >= 14 && i < 18 )
            {
               expResult.push( { a: {"$decimal": left + "." + right } } );
            }
            else if( i >= 18 )
            {
               expResult.push( { a: {"$decimal": left } } );
               expResult.push( { a: {"$decimal": left + "." + right } } );
            }
         }
         else if( dataType == "double" && i < 14 )
         {
            expResult.push({a: parseFloat( left )});
         }
         else if( dataType == "int32" && i < 9 )
         {
            expResult.push({a: parseInt( left )});
         }
      }
   }
   return expResult;
}
function executeFor( expResult, data )
{
   for( var i = 0; i < 20; i++ )
   {
      expResult.push( data );
   }
   return expResult;
}
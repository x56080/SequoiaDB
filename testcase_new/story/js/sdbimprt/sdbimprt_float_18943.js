/************************************************************************
*@Description:  seqDB-18943: 底数的整数位和小数位全为0，指数为309（如00.00e+309） 
*@Author     :  2019-8-8  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18943";
   var csvFile = tmpFileDir + clName + ".csv";
   var jsonFile = tmpFileDir + clName + ".json";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   prepareDate( jsonFile );
   
   println( "\n---data type int32 to import csv file." );
   var fields = "a int";   
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 80 );
   dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();
   
   println( "\n---data type int 64 to import csv file." );
   var fields = "a long";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 80 );
   dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   println( "\n---data type double to import csv file." );
   var fields = "a double";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 80 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();
   
   //SEQUOIADBMAINSTREAM-5074
   /* println( "\n---data type decimal to import csv file." );
   var fields = "a decimal";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 80 );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();*/

   println( "\n---data type double、decimal to import json file." );
   var fields = "a";   
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 80 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   
   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate( typeFile )
{
   var file = new File( typeFile );
   var left = "";
   var right = "";
   for( var i = 0; i < 20; i++ )
   {
      left = left + "0";
      right = right + "0";
      if( typeFile.substring(typeFile.indexOf(".")+1, typeFile.length ) == "csv" )
      {
         file.write( left + "." + right + "e+308"  + "\n" );
         file.write( left + "." + right + "e-308"  + "\n" );
         file.write( left + "." + right + "e+309"  + "\n" );
         file.write( left + "." + right + "e-309"  + "\n" );
      }
      else
      {
         file.write( '{ a:' + left + '.' + right + "e+308" + ' }\n' );
         file.write( '{ a:' + left + '.' + right + "e-308" + ' }\n' );
         file.write( '{ a:' + left + '.' + right + "e+309" + ' }\n' );
         file.write( '{ a:' + left + '.' + right + "e-309" + ' }\n' );
      }
  }
   file.close();
}

function getExpResult( dataType )
{
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      for(var j = 0; j < 4; j++)
      {
         if( dataType === "decimal")
         { 
            expResult.push({ a: {"$decimal": "0" }});
         }
         else
         {
            expResult.push({ a: 0 });
         }
      }
   }
   return expResult;
}

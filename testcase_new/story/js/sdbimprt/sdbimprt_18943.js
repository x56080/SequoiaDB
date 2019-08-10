/************************************************************************
*@Description:  seqDB-18943: 底数的整数位和小数位全为0，指数为309（如00.00e+309） 
*@Author     :  2019-8-8  zhaoxiaoni
************************************************************************/
//SEQUOIADBMAINSTREAM-4800
//main();
function main()
{
   var clName = "cl_18943";
   var csvFile = tmpFileDir + clName + ".csv";
   var jsonFile = tmpFileDir + clName + ".json";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   prepareDate( jsonFile );
   
   println( "\n---data type double、decimal to import csv file." );
   var fields = "a";   
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields );
   checkImportRC( rcResults, 400 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.remove();
   
   println( "\n---data type double、decimal to import json file." );
   var fields = "a";   
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 800 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   
   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate( typeFile )
{
   var file = new File( typeFile );
   var left = "";
   for( var i = 0; i < 620; i++ )
   {
      var right = "";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         file.write( left + "." + right + "e" + (309-i) + "\n" );
      }
   }
}

function getExpResult( dataType )
{
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      var decimalDate = "0.";
      for( var j = 0; j < 20; j++ )
      {
         decimalDate = decimalDate + "0";
         if( dataType == "decimal" )
         {
            expResult.push({ a: { "$decimal": decimalDate }});
         }
         else
         {
            expResult.push({ a: 0 });
         }
      }
   }
   return expResult;
}
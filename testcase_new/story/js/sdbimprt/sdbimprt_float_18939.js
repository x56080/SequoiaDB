/************************************************************************
*@Description:  seqDB-18939: 整数位不为0，小数位后x位为0（如10.01）  
*@Author     :  2019-8-7  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18939";
   var csvFile = tmpFileDir + clName + ".csv";
   var jsonFile = tmpFileDir + clName + ".json";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   prepareDate( jsonFile );
   
   println( "\n---specify data type double、decimal to import csv file." );
   var fields = "a";   
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields );
   checkImportRC( rcResults, 400 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();
   
   println( "\n---specify data type double、decimal to import json file." );
   var fields = "a";   
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 400 );
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
   for( var i = 0; i < 20; i++ )
   {
      var right = "1";
      left = left + "1";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         if( typeFile.substring(typeFile.indexOf(".")+1, typeFile.length ) == "csv" )
         {
            file.write( left + "." + right + "\n" );
         }
         else
         {
            file.write( '{ a:' + left + '.' + right + ' }\n' ); 
         }
      }
   }
   file.close();
}

function getExpResult( dataType )
{
   var expResult = []; 
   var left = "";
   for( var i = 0; i < 20; i++ )
   {
      var right = "1";
      left = left + "1";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         //有效数字超过15位自动识别为decimal
         if( dataType == "decimal" && i >=14 )
         {
            expResult.push( { a: {"$decimal": left + "." + right } } );
         }
         else if( dataType == "double" && i < 14 )
         {
            expResult.push({a: parseFloat( left + ".1" )});
         }
      }
   }
   return expResult;
}
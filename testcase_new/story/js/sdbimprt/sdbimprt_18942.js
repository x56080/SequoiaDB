/************************************************************************
*@Description:  seqDB-18942: 整数位前n位后m位，小数位前x位后y位，n/m/x/y取值随机      
*@Author     :  2019-8-7  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18942";
   var csvFile = tmpFileDir + clName + ".csv";
   var jsonFile = tmpFileDir + clName + ".json";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   
   var expResults = prepareDate( csvFile );
   println( "\n---data type int32、int64、double、decimal to import csv file." );
   var fields = "a";   
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields );
   checkImportRC( rcResults, 2000 );
   dataType = "int32";
   var expResult = getExpResult( dataType, expResults );
   checkResult( cl, dataType, expResult );
   dataType = "int64";
   var expResult = getExpResult( dataType, expResults );
   checkResult( cl, dataType, expResult );
   dataType = "double";
   var expResult = getExpResult( dataType, expResults );
   checkResult( cl, dataType, expResult );
   dataType = "decimal";
   var expResult = getExpResult( dataType, expResults );
   checkResult( cl, dataType, expResult );
   cl.remove();
   
   expResults = prepareDate( jsonFile );
   println( "\n---data type int32、int64、double、decimal to import json file." );
   var fields = "a";   
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 2000 );
   dataType = "int32";
   var expResult = getExpResult( dataType, expResults );
   checkResult( cl, dataType, expResult );
   dataType = "int64";
   var expResult = getExpResult( dataType, expResults );
   checkResult( cl, dataType, expResult );
   dataType = "double";
   var expResult = getExpResult( dataType, expResults );
   checkResult( cl, dataType, expResult );
   dataType = "decimal";
   var expResult = getExpResult( dataType, expResults );
   checkResult( cl, dataType, expResult );
   
   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate( typeFile )
{
   var expResults = {};
   var expResult_int32 = [];
   var expResult_int64 = [];
   var expResult_double = [];
   var expResult_decimal = [];
   var file = new File( typeFile );
   for( var i = 0; i < 1000; i++ )
   {
      var left = getRandom();
      var right = getRandom().toString();
      while( right.substring( right.length - 1 ) == '0' )
      {
         right = right.substring( 0, right.length-1 );
      }
      if( typeFile.substring( typeFile.indexOf(".")+1, typeFile.length ) == "csv" )
      {
         file.write( left + "\n" );
         file.write( left + "." + right + "\n" );
      }
      else
      {
         file.write( '{ a:' + left + ' }\n' );
         file.write( '{ a:' + left + '.' + right + ' }\n' ); 
      }
      if( left < 2147483647 )
      {
         expResult_int32.push( { a: parseInt( left ) } );
      }
      //js能表示的最大整数9007199254740992
      if( left >= 2147483647 && left <= 9007199254740992 )
      {
         expResult_int64.push( { a: left } );
      }
      //整数9007199254740992到9223372036854775807范围内使用$numberLong格式表示
      if( left > 9007199254740992 && left < 9223372036854775807 )
      {
         expResult_int64.push( { a: { "$numberLong" : left.toString() } } );
      }
      if( ( left.toString().length + right.toString().length ) <= 15 )
      {
         expResult_double.push( { a: parseFloat( left + "." + right ) } );
      }
      if( left >= 9223372036854775807)
      {
         expResult_decimal.push( { a: { "$decimal" : left + "" } } );
      }
      if( ( left.toString().length + right.toString().length ) > 15 )
      {
         expResult_decimal.push( { a: { "$decimal" : left + "." + right } } );
      }
   }
   expResults["int32"] = expResult_int32;
   expResults["int64"] = expResult_int64;
   expResults["double"] = expResult_double;
   expResults["decimal"] = expResult_decimal;
   return expResults;
}

function getExpResult( dataType, expResults )
{
   if( dataType == "int32" )
   {
      return expResults["int32"];
   }
   else if( dataType == "int64" )
   {
      return expResults["int64"];
   }
   else if( dataType == "double" )
   {
      return expResults["double"];
   }
   else if( dataType == "decimal" )
   {
      return expResults["decimal"];
   }
}
function getRandom()
{
   //获取20位以内的随机整数
   var num = Math.ceil( Math.random()*20 );
   var random = Math.floor( ( Math.random()*9+1 )*Math.pow(10, num-1) );
   return random;
}
/************************************************************************
*@Description:  seqDB-18935: 整数位前n位后m位为0，小数位前x位后y位为0（如10.010）     
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18935_json";
   var jsonFile = tmpFileDir + clName + ".json";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );
   
   println( "\n---data type double, decimal to import json file." );
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 20 );
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
   var right = "1";
   for(var i=0; i<10; i++)
   {
      file.write( '{ a: ' + left + '.' + right + ' }\n' );
      file.write( '{ a: { "$decimal": "' + left + '.' + right + '" } }\n' );
      left = "0" + left + "0"; 
      right = "0" + right + "0";
   } 
   file.close();
}

function getExpResult( dataType )
{
   var expResult = []; 
   var expResult = []; 
   var left = "1";
   var right = "1";
   for(var i=0; i<10; i++)
   {
      if( dataType == "double" && i < 7)
      {
         expResult.push({ a: parseFloat( left + "." + right )});
      }
      else if( dataType == "decimal" && i < 7)
      {
         expResult.push( { a: {"$decimal": left + "." + right } } );
      }
      else if( dataType == "decimal" && i >= 7)
      {
         expResult.push({ a: {"$decimal": left + "." + right }});
         expResult.push({ a: {"$decimal": left + "." + right }});
      }
      left = left + "0"; 
      right = "0" + right + "0";
   }
   return expResult;
}
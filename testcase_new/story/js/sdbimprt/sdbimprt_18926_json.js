/************************************************************************
*@Description:  seqDB-18926: 整数位前n位为0，小数位全不为0（如01.11）    
*@Author     :  2019-8-5  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18926_json";
   var jsonFile = tmpFileDir + clName + ".json";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );
   
   println( "\n---data type int32, int64, double, decimal to import json file." );
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 800 );
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
      left = "0" + left;
      for( var j = 0; j < 20; j++ )
      {
         right = right + "1";
         file.write( '{ a: { "$decimal": "' + left + '.' + right + '" } }\n' ); 
         file.write( '{ a: ' + left + '.' + right + ' }\n' );
      }   
   }
}

function getExpResult( dataType )
{
   var expResult = []; 
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "1";
         var decimalDate = "1." + right;
         var doubleData = parseFloat( "1." + right );
         if( dataType == "decimal" && j < 14 )
         {
            expResult.push({ a: { "$decimal": decimalDate }});
         }
         else if( dataType == "decimal" && j >= 14 )
         {
            expResult.push({ a: { "$decimal": decimalDate }});
            expResult.push({ a: { "$decimal": decimalDate }});
         }
         else if( dataType == "double" && j < 14 )
         {
            expResult.push({ a: doubleData });
         }
      }
   } 
   return expResult;
}
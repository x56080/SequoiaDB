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
   checkImportRC( rcResults, 40 );
   var expResult = getExpResult( "double" );
   checkResult( cl, "double", expResult );
   var expResult = getExpResult( "decimal" );
   checkResult( cl, "decimal", expResult );
   
   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate( typeFile )
{
   var file = new File( typeFile );
   var left = "10";
   var right = "01000000000000000000";
   for(var i=0; i<10; i++)
   {
      left = "0" + left; 
      file.write( "{ a:" + left + "." + right + "}\n" );
   }
   
   left = "01";
   right = "00000000000000000010";
   for(var i=0; i<10; i++)
   {
      left = left + "0"; 
      file.write( "{ a:" + left + "." + right + "}\n" );
   } 
   
   left = "00000000000000000010";
   right = "01";
   for(var i=0; i<10; i++)
   {
      right = right + "0"; 
      file.write( "{ a:" + left + "." + right + "}\n" );
   }    
   
   left = "010000000000000000000";
   right = "10";
   for(var i=0; i<10; i++)
   {
      right = right + "0"; 
      file.write( "{ a:" + left + "." + right + "}\n" );
   } 
   file.close();
}

function getExpResult( dataType )
{
   var expResult = []; 
   if( dataType === "double" )
   {
      for(var i = 0; i < 20; i++)
      {
         expResult.push({ a: 10.01 });
      }
   }
   else
   {
      var left = "1";
      var right = "00000000000000000010";
      for(var i=0; i<10; i++)
      {
         left = left + "0"; 
         expResult.push({a: { "$decimal": left + "." + right }});
      } 
   
      left = "10000000000000000000";
      right = "10";
      for(var i=0; i<10; i++)
      {
         right = right + "0"; 
         expResult.push({a: { "$decimal": left + "." + right }});
      }     
   }
   return expResult;
}
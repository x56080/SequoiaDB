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
   checkImportRC( rcResults, 20000 );
   var expResult = getExpResult( "double" );
   checkResult( cl, "double", expResult );
   var expResult = getExpResult( "decimal" );
   checkResult( cl, "decimal", expResult );
   
   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate( typeFile )
{
   var file = new File( typeFile );
   var leftR = "";
   for( var i = 0; i < 10; i++ )
   {
      var leftL = "";
      leftR = leftR + "0";
      for( var j = 0; j < 10; j++ )
      {
         var rightL = "";
         leftL = leftL + "0";
         for( var k = 0; k < 10; k++ )
         {
            var rightR = "";
            rightL = rightL + "0";
            for( var l = 0; l < 10; l++ )
            { 
               rightR = rightR + "0";
               var left = leftL + "1" + leftR;
               var right = rightL + "1" + rightR;
               file.write( '{ a: ' + left + '.' + right + ' }\n' );
               file.write( '{ a: { "$decimal": "' + left + '.' + right + '" } }\n' ); 
            }
         }
      }
   }
}

function getExpResult( dataType )
{
   var expResult = []; 
   var leftR = "";
   for( var i = 0; i < 10; i++ )
   {
      var leftL = "";
      leftR = leftR + "0";
      for( var j = 0; j < 10; j++ )
      {
         var rightL = "";
         leftL = leftL + "0";
         for( var k = 0; k < 10; k++ )
         {
            var rightR = "";
            rightL = rightL + "0";
            for( var l = 0; l < 10; l++ )
            { 
               rightR = rightR + "0";
               var left = leftL + "1" + leftR;
               var right = rightL + "1" + rightR;
               if( dataType == "decimal" && (i+k) < 12 )
               {
                  expResult.push( { a: {"$decimal": "1" + leftR + "." + right } } ); 
               }
               else if( dataType == "decimal" && (i+k) >= 12 )
               {
                  expResult.push( { a: {"$decimal": "1" + leftR + "." + right } } );
                  expResult.push( { a: {"$decimal": "1" + leftR + "." + right } } );
               }
               else if( dataType =="double" && (i+k) < 12 )
               {
                  expResult.push({ a: parseFloat( "1" + leftR + "." + rightL + "1" )});
               }
            }
         }
      }
   }
   return expResult;
}
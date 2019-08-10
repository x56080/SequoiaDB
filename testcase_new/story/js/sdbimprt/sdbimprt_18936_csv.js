/************************************************************************
*@Description:  seqDB-18936: 整数位前n位后m位为0，小数位全不为0（如10.11） 
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18936_csv";
   var csvFile = tmpFileDir + clName + ".csv";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   
   println( "\n---specify data type int32 to import csv file." );
   var fields = "a int";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 1800 );
   var dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.remove();
   
   println( "\n---specify data type int64 to import csv file." );
   var fields = "a long";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 1800 );
   var dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.remove();
   
   println( "\n---specify data type double to import csv file." ); 
   var fields = "a double";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 1800 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.remove();
   
   println( "\n---specify data type decimal to import csv file." );
   var fields = "a decimal";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 1800 );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   
   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate( typeFile )
{
   var file = new File( typeFile );
   var leftR = "";
   for( var i = 0; i < 9; i++ )
   {
      var leftL = "";
      leftR = leftR + "0";
      for( var j = 0; j < 10; j++ )
      {
         var right = "";
         leftL = leftL + "0";
         for( var k = 0; k < 20; k++ )
         {
            right = right + "1";
            var left = leftL + "1" + leftR;
            file.write( left + "." + right + "\n" );
         }
      }
   }
}

function getExpResult( dataType )
{
   var expResult = []; 
   var leftR = "";
   for( var i = 0; i < 9; i++ )
   {
      var leftL = "";
      leftR = leftR + "0";
      for( var j = 0; j < 10; j++ )
      {
         var right = "";
         leftL = leftL + "0";
         for( var k = 0; k < 20; k++ )
         {
            right = right + "1";
            var left = leftL + "1" + leftR;
            if( dataType == "decimal" )
            {
               expResult.push( { a: {"$decimal": "1" + leftR + "." + right } } );
            }
            else if( dataType == "double" )
            {
               expResult.push({ a: parseFloat( parseFloat( "1" + leftR + "." + right ).toFixed( 14-i ))});
            }
            else
            {
               expResult.push( { a : parseInt( "1" + leftR ) } );
            }
         }
      }
   }
   return expResult;
}
/************************************************************************
*@Description:  seqDB-18935: 整数位前n位后m位为0，小数位前x位后y位为0（如10.010）     
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18935_csv";
   var csvFile = tmpFileDir + clName + ".csv";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   
   println( "\n---specify data type int32 to import csv file." );
   var fields = "a int";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 10 );
   var dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();
   
   println( "\n---specify data type int64 to import csv file." );
   var fields = "a long";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 10 );
   var dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();
   
   println( "\n---specify data type double to import csv file." ); 
   var fields = "a double";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 10 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();
   
   println( "\n---specify data type decimal to import csv file." );
   var fields = "a decimal";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 10 );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   
   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate( typeFile )
{
   var file = new File( typeFile );
   var left = "1";
   var right = "1";
   for(var i=0; i<10; i++)
   {
      file.write( left + "." + right + "\n" );
      left = "0" + left + "0"; 
      right = "0" + right + "0";
   }      
   file.close();
}

function getExpResult( dataType )
{
   var expResult = []; 
   var left = "1";
   var right = "1";
   for(var i=0; i<10; i++)
   {
      if( dataType == "decimal" )
      {
         expResult.push( { a: {"$decimal": left + "." + right } } );
      }
      else if( dataType == "double" )
      {
         expResult.push({ a: parseFloat( left + "." + right )});
      }
      else
      {
         expResult.push( { a : parseInt( left ) } );
      }
      left = left + "0"; 
      right = "0" + right + "0";
   }
   return expResult;
}
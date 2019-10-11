/************************************************************************
*@Description:  seqDB-18935: 整数位前n位后m位为0，小数位前x位后y位为0（如10.010）     
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18935_csv_long";
   var csvFile = tmpFileDir + clName + ".csv";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   
   println( "\n---specify data type int64 to import csv file." );
   var fields = "a long";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 40 );
   var dataType = "int64";
   var expResult = getExpResult();
   checkResult( cl, dataType, expResult );
   
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
      file.write( left + "." + right + "\n" );
   }
   
   left = "01";
   right = "00000000000000000010";
   for(var i=0; i<10; i++)
   {
      left = left + "0"; 
      file.write( left + "." + right + "\n" );
   } 
   
   left = "00000000000000000010";
   right = "01";
   for(var i=0; i<10; i++)
   {
      right = right + "0"; 
      file.write( left + "." + right + "\n" );
   }
   
   //当浮点数的整数位小于int64的最大值时，指定int64导入集合后显示为numberLong类型
   left = "01000000000000000000";
   right = "10";
   for(var i=0; i<5; i++)
   {
      right = right + "0"; 
      file.write( left + "." + right + "\n" );
   } 
   
   //当浮点数的整数位大于int64的最大值时，指定int64导入集合后显示为0
   left = "010000000000000000000";
   right = "10";
   for(var i=5; i<10; i++)
   {
      right = right + "0"; 
      file.write( left + "." + right + "\n" );
   }    
   
   file.close();
}

function getExpResult()
{
   var expResult = []; 
   var left = "10";
   for(var i=0; i<10; i++)
   {
      expResult.push( { a: parseInt( left ) } );
   }
   left = "1";
   for(var i=0; i<10; i++)
   {
      left = left + "0"; 
      expResult.push({ a: parseInt( left ) });
   } 
   left = "10";
   for(var i=0; i<10; i++)
   {
      expResult.push({ a: parseInt( left ) });
   } 
   left = "1000000000000000000";
   for(var i=0; i<10; i++)
   {
      if( i < 5 )
      {
         expResult.push({ a: { "$numberLong": left } });
      }
      else
      {
         expResult.push({ a: 0 });
      }
   }      
   return expResult;
}
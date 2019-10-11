/************************************************************************
*@Description:  seqDB-18935: 整数位前n位后m位为0，小数位前x位后y位为0（如10.010）     
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/
main();
function main()
{
   var clName = "cl_18935_csv_int";
   var csvFile = tmpFileDir + clName + ".csv";
   
   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   
   println( "\n---specify data type int32 to import csv file." );
   var fields = "a int";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 29 );
   var dataType = "int32";
   var expResult = getExpResult();
   checkResult( cl, dataType, expResult );
   
   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate( typeFile )
{
   //此用例是指定int32导入集合，当浮点数的整数位有效数字大于int32的最大值时，导入到集合后的数据无规则，
   //因此控制浮点数的整数位有效数字小于int32的最大值，构造文本用例中前三种数据
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
   //控制浮点数的整数位有效数字小于int32的最大值,因此i<9
   for(var i=0; i<9; i++)
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
   for(var i=0; i<9; i++)
   {
      left = left + "0"; 
      expResult.push({ a: parseInt( left ) });
   } 
   left = "10";
   for(var i=0; i<10; i++)
   {
      expResult.push({ a: parseInt( left ) });
   }    
   return expResult;
}
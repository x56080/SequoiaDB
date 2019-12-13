/************************************************************************
*@Description:   seqDB-9109:导入regex类型数据合法
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9115";

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9115.json";
   var srcDatas = "{key:MaxKey()}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"key":{"$maxKey":1}}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}


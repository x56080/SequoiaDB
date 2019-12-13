/************************************************************************
*@Description:   seqDB-9097: 字段末尾存在多余逗号
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9097";

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
   var imprtFile = tmpFileDir + "9097.json";
   var srcDatas = "{a:1,b:'test,',}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"a":1,"b":"test,"}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}




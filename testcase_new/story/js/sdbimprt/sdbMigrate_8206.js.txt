/*******************************************************************************
*@Description : import data that the document auto add one field.
*               such:{a:1}, after insert become:{a:1, field1:null}
*               [SEQUOIADBMAINSTREAM-608]
*@Modify list :
*               2015-3-3  xiaojun Hu  Change
*               2019-12-02  Siqin Chen  Change
*******************************************************************************/

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
   var clName = COMMCLNAME + "_import8206";
   var imprtFile = tmpFileDir + "8206.json";

   commDropCL( db, COMMCSNAME, clName, true, true );
   cmd.run( "rm -rf " + imprtFile );

   var cl = commCreateCL( db, COMMCSNAME, clName );
   testImportData8206( COMMCSNAME, clName, imprtFile, cl );
   checkCLData( cl );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( "rm -rf " + imprtFile );

}

function testImportData8206 ( csName, clName, imprtFile, cl )
{
   var file = fileInit( imprtFile );
   file.write( "13434347996,test,中文测试\n13434347997,test1,中文测试1" );
   var imprtOption = installDir + "bin/sdbimprt " + "--hostname " + COORDHOSTNAME + " --svcname " + COORDSVCNAME +
      " -c " + csName + " -l " + COMMCLNAME + " --file " +
      imprtFile + " --type csv --fields \"acc_no,context,text\" -n 1 ";
   if( false == commIsStandalone( db ) )
      db.setSessionAttr( { "PreferedInstance": "M" } );
   println( imprtOption );
   try
   {
      cmd.run( imprtOption );
   }
   catch( e )
   {
      println( "failed to import records" + e );
      throw e;
   }
}

function checkCLData ( cl )
{
   var cursor = cl.find().sort( { "acc_no": 1 } );
   expRecs = '[{"acc_no":13434347996,"context":test,"text":中文测试},{"acc_no":13434347997,"context":test1,"text":中文测试1}]';
   commCompareResults( cursor, expRecs )
}

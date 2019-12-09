/************************************************************************
*@Description:   seqDB-5404:指定目录导入数据，目录/文件不存在
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5404";
      var cl = readyCL( csName, clName );

      importData( csName, clName );

      cleanCL( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}

function importData ( csName, clName )
{
   println( "\n---Begin to import data and check exec result." );

   try
   {
      var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
         + ' -c ' + csName + ' -l ' + clName
         + ' --type csv'
         + ' --file ' + tmpFileDir + 'aa/bb/cc.csv';
      println( "\nimport notExistFile: \n" + imprtOption );
      cmd.run( imprtOption );
   }
   catch( e )
   {
      var expectE = 127;
      if( e !== expectE )
      {
         throw buildException( "importData", e, 'director is null',
            "[e:" + expectE + "]",
            "[e:" + e + "]" );
      }
   }
}
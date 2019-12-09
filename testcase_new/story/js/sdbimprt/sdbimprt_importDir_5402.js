/************************************************************************
*@Description:   seqDB-5402:指定目录导入json/csv文件数据，目录为空
                 seqDB-6211:file指定导入，目录不存在
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5402";
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
         + ' --type csv --fields a'
         + ' --file ' + tmpFileDir;
      println( "import nullDir: \n" + imprtOption );
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

   try
   {
      var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
         + ' -c ' + csName + ' -l ' + clName
         + ' --type csv --fields a'
         + ' --file ' + tmpFileDir + "notExistsDir";
      println( "\nimport notExistDir: \n" + imprtOption );
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
/************************************************************************
*@Description:   seqDB-5414:记录/字段/字符分隔符使用重复
                      （如记录与字段分隔符重复，记录与字符分隔符重复，记录/字段/字符分隔符全部重复）
*@Author:           2016-7-14  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_5414";
      var cl = readyCL( csName, clName );

      var imprtFile = tmpFileDir + "5414.csv";
      readyData( imprtFile );
      importData( csName, clName, imprtFile );

      cleanCL( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}

function readyData ( imprtFile )
{
   println( "\n---Begin to ready data." );

   var file = fileInit( imprtFile );
   file.write( "atest,btest\n1,1\n2,2" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   println( "\n---Begin to import data and check exec result." );

   try
   {
      var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
         + ' -c ' + csName + ' -l ' + clName
         + ' --type csv --fields a -r "," -e "," -a ","'
         + ' --file ' + imprtFile;
      println( imprtOption );
      var rc = cmd.run( imprtOption );
      println( rc );
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
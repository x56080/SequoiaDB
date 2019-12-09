/************************************************************************
*@Description: seqDB-18598:原始集合数据跨多个复制日志，重放复制日志到文件
*@Author: 2019-6-28  xiaoni huang init
*
* 出版本之前跑，已配置到trunk/testcase_new/CI未运行测试用例.xlsx
************************************************************************/
//main();

function main ()
{
   var sclName1;
   var rtCmd;

   try
   {
      if( commIsStandalone( db ) )
      {
         println( "\nThe mode is standalone." );
         return;
      }

      var groupNames = getDataGroupNames();
      var groupName = groupNames[getRandomInt( 0, groupNames.length )];
      var csName = COMMCSNAME;
      clName = "cl18598_" + getRandomInt( 0, 100 );

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      // ready cl data
      var cl = readyCL( csName, clName, { Group: groupName } );

      var recordsNum = 600000;
      println( "\n---Begin to insert records, recordsNum = " + recordsNum + "." );
      for( k = 0; k < recordsNum; k += 50000 ) 
      {
         var doc = [];
         for( i = 0 + k; i < 50000 + k; i++ )
         {
            doc.push( { a: i, b: "test" + i } );
         };
         cl.insert( doc );
      }

      // ready outputconf for sdbreplay
      var tmpConfName = "sdbreplay_18598.conf";
      getOutputConfFile( groupName, csName, clName, tmpConfName );
      configOutputConfFile( rtCmd, groupName, csName, clName );
      // replay
      var clNameArr = [csName + "." + clName];
      var confPath = tmpFileDir + csName + "." + clName + ".conf";
      execSdbReplay( rtCmd, groupName, clNameArr, "replica", confPath );

      // check results
      // Ensure successful execution, and total count is correct
      println( "\n---Begin to check csv file content." );
      var csvFileName = rtCmd.run( "ls " + tmpFileDir + " | grep " + clName + " | grep csv" ).split( "\n" )[0];
      var csvFilePath = tmpFileDir + csvFileName;
      var command = "awk 'END{print NR}' " + csvFilePath;
      println( command );
      var rcNum = rtCmd.run( command ).split( "\n" )[0];
      if( recordsNum !== Number( rcNum ) )
      {
         throw buildException( "main", null, "[check csv file data number]",
            "[" + recordsNum + "]",
            "[" + rcNum + "]" );
      }

      // clean env
      cleanCL( csName, clName );
      cleanFile( rtCmd );
   }
   catch( e )
   {
      backupFile( rtCmd, clName );
      throw e;
   }
}

function configOutputConfFile ( rtCmd, groupName, csName, clName )
{
   println( "\n---Begin to config outputconf." );
   var fullCLName = csName + "." + clName;
   var targetConfPath = tmpFileDir + fullCLName + ".conf";

   rtCmd.run( "sed -i 's/filePrefix_ori/test_" + groupName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/source_fullCLName_ori/" + fullCLName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/target_fullCLName_ori/" + fullCLName + "_new/g' " + targetConfPath );
}
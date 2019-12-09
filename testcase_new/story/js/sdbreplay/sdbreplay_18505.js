/************************************************************************
*@Description: seqDB-18505:delimiter配置字段间分隔符为任意可见字符
*@Author: 2019-6-28  xiaoni huang init
************************************************************************/
main();

function main ()
{
   var clName;
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
      clName = "cl18505_" + getRandomInt( 0, 100 );

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      // ready cl data
      var cl = readyCL( csName, clName, { Group: groupName } );
      cl.insert( { a: "test" } );
      cl.update( { $set: { a: "test2" } } );
      cl.remove();

      // ready outputconf for sdbreplay
      var fieldType = "MAPPING_STRING";
      var delimiter = getRandomString( getRandomInt( 1, 10 ) )
         .replace( /\\/g, "_newR1_" ).replace( /\"/g, "_newR2_" ).replace( / /g, "_newR3_" );
      readyOutputConfFile( rtCmd, groupName, csName, clName, fieldType, delimiter );
      println( '   delimiter = ' + delimiter );
      // replay
      var clNameArr = [csName + "." + clName];
      execSdbReplay( rtCmd, groupName, clNameArr, "replica" );

      // check results
      var expDataArr = ['"I"' + delimiter + '"test"',
      '"B"' + delimiter + '"test"',
      '"A"' + delimiter + '"test2"',
      '"D"' + delimiter + '"test2"'];
      checkCsvFile( rtCmd, clName, expDataArr );

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
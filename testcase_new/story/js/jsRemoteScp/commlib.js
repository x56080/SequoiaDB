/******************************************************************************
*@Description : get local hostname
*@author      : Liang XueWang
******************************************************************************/
function toolGetLocalhost ()
{
   var cmd = new Cmd();
   var localhost = cmd.run( "hostname" ).split( "\n" )[0];
   return localhost;
}

/******************************************************************************
*@Description : get hosts in cluster
*@author      : Liang XueWang
******************************************************************************/
function toolGetHosts ()
{
   var hosts = [];
   var k = 0;

   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone." );
      db.close();
      return hosts;
   }

   var tmpInfo = db.listReplicaGroups().toArray();
   for( var i = 0; i < tmpInfo.length; i++ )
   {
      var tmpObj = JSON.parse( tmpInfo[i] );
      var tmpArr = tmpObj.Group;
      for( var j = 0; j < tmpArr.length; j++ )
      {
         if( hosts.indexOf( tmpArr[j].HostName ) == -1 )
            hosts[k++] = tmpArr[j].HostName;
      }
   }
   db.close();
   return hosts;
}

/******************************************************************************
*@Description : get a remote hostname in cluster
*               if cluster has no remote host, return localhost
*@author      : Liang XueWang
******************************************************************************/
function toolGetRemotehost ()
{
   var hosts = toolGetHosts();
   var localhost = toolGetLocalhost();
   var remotehost = localhost;
   for( var i = 0; i < hosts.length; i++ )
   {
      if( hosts[i] !== localhost )
      {
         remotehost = hosts[i];
         break;
      }
   }
   return remotehost;
}


//read and write content, check length
function readWriteContentAndCheck ( readFile, writeFile, length, fileSize )
{
   if( typeof ( length ) == "undefined" ) { length = 1024; }
   if( typeof ( fileSize ) == "undefined" ) { fileSize = length; }

   try
   {
      var content = readFile.readContent( length );
      var readLength = content.getLength();
      if( length > fileSize )
      {
         length = fileSize;
      }
      if( readLength !== parseInt( length ) )
      {
         throw "READ_LENGTH_ERROR";
      }

      writeFile.writeContent( content );
      writeFileName = writeFile.getInfo().toObj().filename;
      var writeLength = parseInt( writeFile.stat( writeFileName ).toObj().size );
      writeFile.remove( writeFileName );
      if( writeLength !== parseInt( length ) )
      {
         throw "WRITE_LENGTH_ERROR";
      }
   }
   catch( e )
   {
      throw buildException( "readWriteContentAndCheck()", e, e, length, "readLength:" + readLength + ", writeLength:" + writeLength );
   }
}

//read and write many times
function readWriteContentManyTimes ( readFile, writeFile, length )
{
   try
   {
      readFileName = readFile.getInfo().toObj().filename;
      writeFileName = writeFile.getInfo().toObj().filename;
      var fileSize = parseInt( readFile.stat( readFileName ).toObj().size );
      var times = Math.ceil( fileSize, length );

      for( var i = 0; i < times; i++ )
      {
         try
         {
            var content = readFile.readContent( length );
            writeFile.writeContent( content );
         }
         catch( e )
         {
            if( -9 === e )
            {
               break;
            }
            else
            {
               throw e;
            }
         }
      }

      //check
      var readMd5 = readFile.md5( readFileName );
      var writeMd5 = writeFile.md5( writeFileName );
      writeFile.remove( writeFileName );
      if( readMd5 !== writeMd5 )
      {
         throw "MD5_NOT_SAME";
      }
   }
   catch( e )
   {
      throw buildException( "readWriteContentManyTimes()", e, e, "readMd5:" + readMd5, ", writeMd5:" + writeMd5 );
   }
}

function checkArgumentRead ( readFile, length, errCode )
{
   if( typeof ( errCode ) == "undefined" ) { errCode = -6; }
   try
   {
      var content = readFile.readContent( length );
      throw "EXPECT GET AN ERROR"
   }
   catch( e )
   {
      if( e !== errCode )
      {
         throw buildException( "checkArgumentRead()", e, e, "FAILED", "SUCCESS" );
      }
   }
}

function checkArgumentWrite ( readFile, writeFile, content, length )
{
   try
   {
      writeFile.writeContent( content );
      throw "EXPECT GET AN ERROR";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "writeContent()", e, e, "FAILED", "SUCCESS" );
      }
   }
   writeFile.remove( writeFileName );
}

function toBase64CodeTest ( readFile, actualFile, expectFile, length, cmd )
{
   if( typeof ( length ) == "undefined" ) { length = 1024; }

   try
   {
      var readFileName = readFile.getInfo().toObj().filename;
      var actualFileName = actualFile.getInfo().toObj().filename;
      var expectFileName = expectFile.getInfo().toObj().filename;
      var fileSize = parseInt( readFile.stat( readFileName ).toObj().size );

      var times = Math.ceil( fileSize, length );
      for( var i = 0; i < times; i++ )
      {
         try
         {
            var content = readFile.readContent( length );
            var base64 = content.toBase64Code();
            actualFile.write( base64 );
         }
         catch( e )
         {
            if( -9 === e ) { break; }
         }
      }

      cmd.run( "base64 " + readFileName + "> " + expectFileName + "_tmp" );
      //cmd.run( "sed -i ':a; N; $!ba; s/\\n//g' " + expectFileName ); 
      cmd.run( "cat " + expectFileName + "_tmp |tr -d '\\n' >" + expectFileName );

      //check
      var expectMd5 = expectFile.md5( expectFileName );
      var actualMd5 = actualFile.md5( actualFileName );
      expectFile.remove( expectFileName + "_tmp" );
      expectFile.remove( expectFileName );
      actualFile.remove( actualFileName );
      if( expectMd5 !== actualMd5 )
      {
         throw "MD5_NOT_SAME";
      }
   }
   catch( e )
   {
      throw buildException( "toBase64CodeTest()", e, e, "expectMd5:" + expectMd5, ", actualMd5:" + actualMd5 );
   }
}

function getPermission ( file )
{
   try
   {
      var mode = file._getPermission( "/tmp" );
      if( mode !== 511 )
      {
         throw "MODE_ERROR"
      }
   }
   catch( e )
   {
      throw buildException( "_getPermission()", e, e, 511, mode );
   }
}


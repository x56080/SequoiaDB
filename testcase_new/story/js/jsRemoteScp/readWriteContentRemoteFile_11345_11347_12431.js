/************************************
*@Description:
*@author:      zhaoyu
*@createdate:  2017.4.13
*@testlinkCase:seqDB-11335/seqDB-11337
**************************************/
function main ()
{
   var localhost = toolGetLocalhost();
   println( "localhost:" + localhost );

   var remotehost = toolGetRemotehost();
   println( "remotehost:" + remotehost );
   var remote = new Remote( remotehost, CMSVCNAME );
   var remoteFile = remote.getFile();

   var remoteCmd = remote.getCmd();
   var remoteInstallPath = commGetInstallPath();
   println( "remoteInstallPath:" + remoteInstallPath );
   var readFileName = remoteInstallPath + "/bin/sdbdpsdump";
   println( "read file name :" + readFileName );

   if( !remoteFile.exist( WORKDIR ) )
   {
      remoteFile.mkdir( WORKDIR, 0777 );
   }

   var writeFileName = WORKDIR + "/writeFile_11337";
   var emptyFileName = WORKDIR + "/emptyFile_11337";
   //0 size
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 0 );
   println( "read and write content 0 size success" );

   //default size
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentAndCheck( readFile, writeFile );
   println( "read and write content 1024 size success" );

   //size = 4M
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 4194304 );
   println( "read and write content 4194304 size success" );

   //size = fileLength
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   var fileSize = parseInt( readFile.stat( readFileName ).toObj().size );
   readWriteContentAndCheck( readFile, writeFile, fileSize );
   println( "read and write content " + fileSize + " size success" );

   //size > fileLength
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   var overSize = fileSize + 104857600;
   readWriteContentAndCheck( readFile, writeFile, overSize, fileSize );
   println( "read and write content " + overSize + " size success" );

   //read empty file
   try
   {
      if( remoteFile.exist( emptyFileName ) )
      {
         remoteFile.remove( emptyFileName );
      }
      var emptyFile = remote.getFile( emptyFileName );
      var content = emptyFile.readContent();
      throw "EXPECT GET AN ERROR";
   }
   catch( e )
   {
      if( e !== -9 )
      {
         throw buildException( "readContent()", e, "read empty file", 0, content.getLength() );
      }
   }
   remoteFile.remove( emptyFileName );
   println( "read content from empty file success" );

   //many times read and write, size 1M
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentManyTimes( readFile, writeFile, 102400 );
   println( "many times read and write content 102400 size success" );

   //many times read and write, size 100M
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentManyTimes( readFile, writeFile, 104857600 );
   println( "many times read and write content 104857600 size success" );

   //mode test
   //SDB_FILE_READONLY
   try
   {
      var readFile = remote.getFile( readFileName, 0777, SDB_FILE_READONLY );
      var content = readFile.readContent();
   }
   catch( e )
   {
      throw e;
   }

   try
   {
      var writeFile = remote.getFile( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_READONLY );
      writeFile.writeContent( content );
      throw "NEED_AN_ERROR";
   }
   catch( e )
   {
      if( e !== -3 )
      {
         throw e;
      }
   }
   println( "check mode set SDB_FILE_READONLY success!" );

   //SDB_FILE_WRITEONLY
   try
   {
      var readFile = remote.getFile( readFileName, 0777, SDB_FILE_WRITEONLY );
      var content = readFile.readContent();
      throw "NEED_AN_ERROR";
   }
   catch( e )
   {
      if( e !== -3 )
      {
         throw e;
      }
   }


   try
   {
      var writeFile = remote.getFile( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_WRITEONLY );
      writeFile.writeContent( content );
      var writeLength = parseInt( writeFile.stat( writeFileName ).toObj().size );
      if( writeLength !== 1024 )
      {
         throw "WRITE_LENGTH_ERROR";
      }
      writeFile.remove( writeFileName );
   }
   catch( e )
   {
      throw e;
   }
   println( "check mode set SDB_FILE_WRITEONLY success!" );

   //SDB_FILE_READWRITE
   try
   {
      var readFile = remote.getFile( readFileName, 0777, SDB_FILE_READWRITE );
      var content = readFile.readContent();
      var writeFile = remote.getFile( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_READWRITE );
      writeFile.writeContent( content );
      var writeLength = parseInt( writeFile.stat( writeFileName ).toObj().size );
      if( writeLength !== 1024 )
      {
         println( "writeLength:" + writeLength );
         throw "WRITE_LENGTH_ERROR";
      }

      //SEQUOIADBMAINSTREAM-2661, clear()
      content.clear();
      var readLength = content.getLength();
      if( readLength !== 0 )
      {
         println( "readLength:" + readLength );
         throw "CLEAR_CONTENT_ERROR";
      }

      writeFile.remove( writeFileName );
   }
   catch( e )
   {
      throw e;
   }
   println( "check mode set SDB_FILE_READWRITE success!" );

   //argument check
   //float size
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 1024.88 )
   println( "check float size success" );

   //string
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   checkArgumentRead( readFile, "a" );
   println( "check string size success" );

   //negative int; 
   checkArgumentRead( readFile, -10 );
   checkArgumentRead( readFile, -1023 );
   println( "check negative int size success" );

   //long
   checkArgumentRead( readFile, 9007199254740992, -2 );
   println( "check long size success" );

   //writeContent argument illegal
   //miss argument
   try
   {
      var readFile = remote.getFile( readFileName );
      var content = readFile.readContent();
      if( remoteFile.exist( writeFileName ) )
      {
         remoteFile.remove( writeFileName );
      }
      var writeFile = remote.getFile( writeFileName );
      writeFile.writeContent();
      throw "EXPECT GET AN ERROR";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "writeContent()", e, e, "FAILED", "SUCCESS" );
      }
   }
   remoteFile.remove( writeFileName );
   println( "check miss argument success" );

   //type illegal
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   checkArgumentWrite( readFile, writeFile, "content" );
   println( "check type illegal success" );

   //_getPermission
   getPermission( remoteFile );
   println( "check _getPermission success" );

   //toBase64Code(), set length = 3 multiples
   var actualFileName = WORKDIR + "/tobase64File_11337";
   var expectFileName = WORKDIR + "/base64File_11337";
   var length = 3000000;

   if( remoteFile.exist( actualFileName ) )
   {
      remoteFile.remove( actualFileName );
   }
   if( remoteFile.exist( expectFileName ) )
   {
      remoteFile.remove( expectFileName );
   }

   var readFile = remote.getFile( readFileName );
   var actualFile = remote.getFile( actualFileName );
   var expectFile = remote.getFile( expectFileName );
   toBase64CodeTest( readFile, actualFile, expectFile, length, remoteCmd );
   println( "set length = 3 multiples toBase64Code success " );

}
main(); 

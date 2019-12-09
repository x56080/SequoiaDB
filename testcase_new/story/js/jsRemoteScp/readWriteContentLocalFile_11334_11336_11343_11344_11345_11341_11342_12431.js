/************************************
*@Description:
*@author:      zhaoyu
*@createdate:  2017.4.12
*@testlinkCase:seqDB-11334/seqDB-11336/seqDB-11343/seqDB-11344/seqDB-11345/seqDB-11341/seqDB-11342
**************************************/
function main ()
{
   //get path
   var cmd = new Cmd();
   var localInstallPath = commGetInstallPath();
   var readFileName = localInstallPath + "/bin/sdbdpsdump";
   println( "read file name :" + readFileName );

   if( !File.exist( WORKDIR ) )
   {
      File.mkdir( WORKDIR, 0777 );
   }

   var writeFileName = WORKDIR + "/writeFile_11336";
   var emptyFileName = WORKDIR + "/emptyFile_11336";

   if( File.exist( writeFileName ) )
   {
      File.remove( writeFileName );
   }

   //0 size
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 0 );
   println( "read and write content 0 size success" );

   //default size
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentAndCheck( readFile, writeFile );
   println( "read and write content 1024 size success" );

   //size = 4M
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 4194304 );
   println( "read and write content 4194304 size success" );

   //size = fileLength
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   var fileSize = parseInt( File.stat( readFileName ).toObj().size );
   readWriteContentAndCheck( readFile, writeFile, fileSize );
   println( "read and write content " + fileSize + " size success" );

   //size > fileLength
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   var overSize = fileSize + 104857600;
   readWriteContentAndCheck( readFile, writeFile, overSize, fileSize );
   println( "read and write content " + overSize + " size success" );

   //read empty file
   try
   {
      if( File.exist( emptyFileName ) )
      {
         File.remove( emptyFileName );
      }
      var emptyFile = new File( emptyFileName );
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
   File.remove( emptyFileName );
   println( "read content from empty file success" );

   //many times read and write, size 1M
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentManyTimes( readFile, writeFile, 102400 );
   println( "many times read and write content 102400 size success" );

   //many times read and write, size 100M
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentManyTimes( readFile, writeFile, 104857600 );
   println( "many times read and write content 104857600 size success" );

   //mode test
   //SDB_FILE_READONLY
   try
   {
      var readFile = new File( readFileName, 0777, SDB_FILE_READONLY );
      var content = readFile.readContent();
   }
   catch( e )
   {
      throw e;
   }

   try
   {
      var writeFile = new File( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_READONLY );
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
      var readFile = new File( readFileName, 0777, SDB_FILE_WRITEONLY );
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
      var writeFile = new File( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_WRITEONLY );
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
      var readFile = new File( readFileName, 0777, SDB_FILE_READWRITE );
      var content = readFile.readContent();
      var writeFile = new File( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_READWRITE );
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
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 1024.88 )
   println( "check float size success" );

   //string
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
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
      var readFile = new File( readFileName );
      var content = readFile.readContent();
      if( File.exist( writeFileName ) )
      {
         File.remove( writeFileName );
      }
      var writeFile = new File( writeFileName );
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
   File.remove( writeFileName );
   println( "check miss argument success" );

   //type illegal
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   checkArgumentWrite( readFile, writeFile, "content" );
   println( "check type illegal success" );

   //_getPermission
   getPermission( File );
   println( "check _getPermission success" );

   //toBase64Code(), set length = 3 multiples
   var actualFileName = WORKDIR + "/tobase64File_11336";
   var expectFileName = WORKDIR + "/base64File_11336";
   var length = 3000000;

   if( File.exist( actualFileName ) )
   {
      File.remove( actualFileName );
   }
   if( File.exist( expectFileName ) )
   {
      File.remove( expectFileName );
   }

   var readFile = new File( readFileName );
   var actualFile = new File( actualFileName );
   var expectFile = new File( expectFileName );
   var cmd = new Cmd();
   toBase64CodeTest( readFile, actualFile, expectFile, length, cmd );
   println( "set length = 3 multiples toBase64Code success " );

}
main(); 

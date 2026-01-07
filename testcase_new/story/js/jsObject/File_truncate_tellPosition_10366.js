/************************************
*@Description：截断File对象的内容，获取文件游标偏移量
*@author：     fangjiabin
*@createDate:  2026.01.07
**************************************/


main( test );

function test ()
{
   var loaclFilePath = WORKDIR + "/localFile10366/";
   var remoteFilePath = WORKDIR + "/remoteFile10366/";
   var fileName = "file10366";

   test1( fileName, loaclFilePath, true );

   test1( fileName, remoteFilePath, false );
}

function test1( fileNameBase, filePath, isLocal )
{
   var content = "123456789";
   var contentSize = content.length;

   removeFile ( filePath, isLocal )

   var fileName = fileNameBase + "_1";
   var fileFullName = filePath + fileName;
   var file = createFile( filePath, fileName, isLocal );
   file.write( content );
   file.truncate( 0 );
   assert.equal( file.tellPosition(), 0 );
   file.write( content );
   assert.equal( file.tellPosition(), contentSize );
   file.close();
   assert.equal( getFileSize( fileFullName, isLocal ), contentSize );
   checkTruncateResult( filePath, fileName, content, isLocal );

   fileName = fileNameBase + "_2";
   fileFullName = filePath + fileName;
   file = createFile( filePath, fileName, isLocal );
   file.write( content );
   file.truncate( 2 );
   assert.equal( file.tellPosition(), 2 );
   file.write( "3456789" );
   assert.equal( file.tellPosition(), contentSize );
   file.close();
   assert.equal( getFileSize( fileFullName, isLocal ), contentSize );
   checkTruncateResult( filePath, fileName, content, isLocal );

   fileName = fileNameBase + "_3";
   fileFullName = filePath + fileName;
   file = createFile( filePath, fileName, isLocal );
   file.write( content );
   file.truncate( 20 );
   assert.equal( file.tellPosition(), contentSize );
   file.write( content );
   assert.equal( file.tellPosition(), contentSize * 2 );
   file.close();
   assert.equal( getFileSize( fileFullName, isLocal ), 20 );
   checkTruncateResult( filePath, fileName, content + content, isLocal );

   removeFile ( filePath, isLocal )
}

function createFile ( filePath, fileName, isLocal )
{
   if( isLocal )
   {
      File.mkdir( filePath );
      var file = new File( filePath + fileName );
   }
   else
   {
      var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
      remote.getFile().mkdir( filePath );
      var file = remote.getFile( filePath + fileName );
   }

   return file;
}

function checkTruncateResult ( filePath, fileName, expContent, isLocal )
{
   if( isLocal )
   {
      var file = new File( filePath + fileName );
   }
   else
   {
      var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
      var file = remote.getFile( filePath + fileName );
   }

   if( expContent != null )
   {
      var actStr = file.read();
      file.close();
      assert.equal( actStr, expContent );
   }
   else
   {
      assert.tryThrow( SDB_EOF, function()
      {
         file.read();
      } );
   }

}

function removeFile ( filePath, isLocal )
{
   try
   {
      if( isLocal )
      {
         File.remove( filePath );
      }
      else
      {
         var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
         remote.getFile().remove( filePath );
      }
   }
   catch( e )
   {
      if ( e != -4 )
      {
         println( "Failed to remove file[" + filePath + "], isLocal[" + isLocal + "], rc: " + e ) ;
         throw e ;
      }
   }
}

function getFileSize( fileFullName, isLocal )
{
   var fileSize = 0;
   if( isLocal )
   {
      fileSize = File.getSize( fileFullName );
   }
   else
   {
      var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
      fileSize = remote.getFile().getSize( fileFullName );
   }
   return fileSize;
}
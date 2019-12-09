/****************************************************************
*@Description : test js object File function: get file size
*               TestLink : 12429 获取文件大小
*@auhor       : Liang XueWang
*               2017-08-17
*****************************************************************/
FileTest.prototype.testGetSize = function()
{
   this.init();

   var notExistFile = "/tmp/notExistFile_12429.txt";
   var noPermFile = "/tmp/noPermFile_12429.txt";
   var normalFile = "/tmp/normalFile_12429.txt";
   var bigFile = "/tmp/bigFile2G_12429.txt";
   var file;
   var size1, size2;

   // 测试获取不存在文件的大小
   try
   {
      this.file.getSize( notExistFile );
      throw 0;
   }
   catch( e )
   {
      if( e !== -4 )
      {
         throw buildException( "testGetSize", e, "get size of not exist file " + this,
            -4, e );
      }
   }

   // 测试获取无权限文件的大小（正常获取）
   if( this.isLocal )
   {
      file = new File( noPermFile );
   }
   else
   {
      file = this.remote.getFile( noPermFile );
   }
   file.write( "abcde" );
   file.close();
   this.file.chmod( noPermFile, 0000 );
   size1 = parseInt( this.file.stat( noPermFile ).toObj().size );
   size2 = this.file.getSize( noPermFile );
   if( size2 !== size1 )
   {
      throw buildException( "testGetSize", null, "check get size " + this,
         size1, size2 );
   }
   this.file.remove( noPermFile );

   // 测试获取正常文件大小
   if( this.isLocal )
   {
      file = new File( normalFile );
   }
   else
   {
      file = this.remote.getFile( normalFile );
   }
   file.write( "abcde" );
   file.close();
   size1 = parseInt( this.file.stat( normalFile ).toObj().size );
   size2 = this.file.getSize( normalFile );
   if( size2 !== size1 )
   {
      throw buildException( "testGetSize", null, "check get size " + this,
         size1, size2 );
   }
   this.file.remove( normalFile );

   // 测试获取非文本文件的大小 如/usr/bin/who
   var binaryFile = "/usr/bin/who";
   size1 = parseInt( this.file.stat( binaryFile ).toObj().size );
   size2 = this.file.getSize( binaryFile );
   if( size2 !== size1 )
   {
      throw buildException( "testGetSize", null, "check get binary file size " + this,
         size1, size2 );
   }

   // 测试获取大小超过int边界的文件大小
   try
   {
      this.cmd.run( "dd if=/dev/zero of=" + bigFile + " bs=1G count=2" );
   }
   catch( e )
   {
      println( "disk have no space to create big file of 2G" );
      this.cmd.run( "rm -rf " + bigFile );
      return;
   }
   size1 = parseInt( this.file.stat( bigFile ).toObj().size );
   size2 = this.file.getSize( bigFile );
   if( size2 !== size1 || size1 !== 2147483648 )
   {
      throw buildException( "testGetSize", null, "check get big file size " + this,
         size1 + " " + 2147483648, size2 );
   }
   this.file.remove( bigFile );

   this.release();
}

function main ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var ft1 = new FileTest( localhost, CMSVCNAME );     // 本地File类类型
   var ft2 = new FileTest( remotehost, CMSVCNAME );    // 远程File类类型

   var fts = [ft1, ft2];

   for( var i = 0; i < fts.length; i++ )
   {
      // 测试文件大小
      fts[i].testGetSize();
   }
}

main()
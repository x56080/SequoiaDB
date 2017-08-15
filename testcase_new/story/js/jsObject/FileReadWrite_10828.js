/******************************************************************************
*@Description : test js object File function: read write close seek
*               TestLink : 10828 读取File对象文件内容
*                          10829 向File对象写内容
*                          10830 设置文件指针位置
*                          10831 设置文件指针位置，超过边界
*                          10832 关闭文件
*@auhor       : Liang XueWang
******************************************************************************/

// 测试读写文件，偏移读，关闭文件
FileTest.prototype.testReadWrite = function()
{
   this.init() ;
   
   var content = generateContent( 'a', 1025 ) ;
   this.file.write( content ) ;     // 写文件
   
   this.file.seek( 0, 'b' ) ;       
   var readPart = this.file.read( 4 ) ;   // 偏移读部分字符
   if( readPart !== "aaaa" )
   {
      throw buildException( "testReadWrite", null, 
                            "test read part " + this, "aaaa", readPart ) ;
   }
   
   this.file.seek( 0, 'b' ) ;
   var readMax = this.file.read() ;       // 偏移读1024个字符
   if( readMax !== generateContent( 'a', 1024 ) )
   {
      throw buildException( "testReadWrite", null, 
            "test read 1024 " + this, generateContent( 'a', 1024 ), readMax ) ;
   }
   
   var readRest = this.file.read() ;      // 读取剩余字符
   if( readRest !==  'a' )
   {
      throw buildException( "testReadWrite", null, 
                            "test read rest " + this, 'a', readRest ) ;
   }
   
   this.file.close() ;      // 关闭文件
   checkClose( this.file ) ;
   
   this.release() ;
}

// 测试偏移超出文件边界，仅文件头有边界
FileTest.prototype.testSeekBoundary = function()
{
   this.init() ;
   
   try
   {
      this.file.seek( -1, 'b' ) ;  // 测试偏移超出文件头边界
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "testSeekBoundary", e, 
                               "test exceed head boundary " + this, -6, e ) ;
      }
   }
   
   try
   {
      this.file.seek( 0, 'b' ) ;
      this.file.write( "abcdefg" ) ;   // 测试偏移到文件尾部读取
      this.file.seek( 0, 'e' ) ;
      this.file.read() ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -9 )
      {
         throw buildException( "testSeekBoundary", e, 
                               "test seek end and read " + this, -9, e ) ;
      }
   }
   
   this.release() ;
}

/******************************************************************************
*@Description : generate content: repeat ch size times like 'aaaaa...'
*@author      : Liang XueWang            
******************************************************************************/
function generateContent( ch, size )
{
   var str = ch ;
   for( var i = 0;i < size-1;i++ )
      str += ch ;
   return str ;
}

/******************************************************************************
*@Description : check file close: write after close
*@author      : Liang XueWang            
******************************************************************************/
function checkClose( file )
{
   try
   {
      file.write( 'abcd' ) ;
      throw "write after file close should be failed" ;
   }
   catch( e )
   {
      if( e !== -1 )
      {
         throw buildException( "checkClose", e, "write after close", -1, e ) ;
      }
   }
}   


function main()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   var filename = "/tmp/testFileReadAndWrite10828.txt" ;
   var ft1 = new FileTest( localhost, CMSVCNAME, filename ) ;   // 本地file对象
   var ft2 = new FileTest( remotehost, CMSVCNAME, filename ) ;  // 远程file对象
   
   var fts = [ ft1, ft2 ] ;
   
   for( var i = 0; i < fts.length;i++ )
   {
      // 测试读写文件，偏移读
      fts[i].testReadWrite() ;
      
      // 测试偏移超过边界
      fts[i].testSeekBoundary() ;
   }
}

main()
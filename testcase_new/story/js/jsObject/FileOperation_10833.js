/******************************************************************************
*@Description : test js object File function: mkdir move copy remove
*               TestLink : 10837 新建目录
*                          10836 移动文件
*                          10835 拷贝文件
*                          10833 删除文件
*@auhor       : Liang XueWang
******************************************************************************/
// 测试创建目录，移动文件，拷贝文件，删除文件
FileTest.prototype.testFileOperation = function()
{
   this.init() ;
   
   var tmpDirName = "/tmp/FileTest" ;
   var tmpFileName = "/tmp/FileTest/tmpFile" ;
   var tmpFile ;
   
   this.file.mkdir( tmpDirName ) ;   // 创建目录
   checkMkdir( this.cmd, tmpDirName ) ;
   if( this.isLocal )
      tmpFile = new File( tmpFileName, 0644 ) ;
   else
      tmpFile = this.remote.getFile( tmpFileName, 0644 ) ;
   
   try
   {   
      this.file.move( tmpFileName, tmpFileName + ".move" ) ; // 移动文件
      checkMove( this.cmd, tmpFileName, tmpFileName + ".move" ) ;
      this.file.move( tmpFileName + ".move", tmpFileName ) ; 
      
      this.file.copy( tmpFileName, tmpFileName + ".copy" ) ;  // 拷贝文件
      checkCopy( this.cmd, tmpFileName, tmpFileName + ".copy" ) ;
      
      this.file.remove( tmpFileName ) ;      // 删除文件
      checkRemove( this.cmd, tmpFileName ) ;
      this.file.remove( tmpFileName + ".copy" ) ;
   }
   catch( e )
   {
      throw e ;
   }
   finally
   {
      this.file.remove( tmpDirName ) ;
   } 
   
   this.release() ;  
}

// 测试拷贝文件时指定权限
FileTest.prototype.testCopyWithMode = function()
{
   this.init() ;
   
   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname ) ;
   var srcFile = sdbDir[0] + "/bin/sdb" ;      // -rwxr-xr-x
   var dstFile = sdbDir[0] + "/bin/sdb.bak" ;
   var mode = this.file.stat( srcFile ).toObj().mode.slice( 0, 10 ) ;
   if( mode !== "rwxr-xr-x" )
   {
      println( srcFile + " mode: " + mode + " " + this ) ;
      this.release() ;
      return ;
   }
   var umask = this.file.getUmask( '8' ) ;
   if( umask !== "0022" )
   {
      println( "umask: " + umask + " " + this ) ;
      this.release() ;
      return ;
   }
   this.cmd.run( "rm -rf " + dstFile ) ;
   
   // 测试目标文件不存在时指定权限需要与umask运算
   this.file.copy( srcFile, dstFile, false, 0733 ) ;   // 0733 - 0022 = 0711
   var dstFileMode = this.file.stat( dstFile ).toObj().mode.slice( 0, 10 ) ;
   if( dstFileMode !== "rwx--x--x" )
   {
      throw buildException( "testCopyWithMode", null, "copy file when " + 
            srcfile + " not exist " + this, "rwx--x--x", dstFileMode ) ; 
   }
   
   // 测试目标文件存在时设置权限无效，保留原文件权限
   this.file.copy( srcFile, dstFile, true, 0777 ) ;   
   var dstFileMode = this.file.stat( dstFile ).toObj().mode.slice( 0, 10 ) ;
   if( dstFileMode !== "rwx--x--x" )
   {
      throw buildException( "testCopyWithMode", null, "copy file when " + 
            srcfile + " exist " + this, "rwx--x--x", dstFileMode ) ; 
   }
   
   // 测试目标文件存在且isReplace为false时，拷贝失败
   try
   {
      this.file.copy( srcFile, dstFile, false ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -5 )
      {
         throw buildException( "testCopyWithMode", e, "copy file when " +
               dstFile + " exist and isReplace false " + this, -5, e ) ;
      }
   }
   
   this.cmd.run( "rm -rf " + dstFile ) ;
   
   this.release() ;
}

/******************************************************************************
*@Description : check mkdir
*@author      : Liang XueWang            
******************************************************************************/
function checkMkdir( cmd, dirName )
{
   try
   {
      cmd.run( "ls -al " + dirName ) ;
   }
   catch( e )
   {
      throw buildException( "checkMkdir", e ) ;
   }
}

/******************************************************************************
*@Description : check move file
*@author      : Liang XueWang            
******************************************************************************/
function checkMove( cmd, oldFile, newFile )
{
   try
   {
      cmd.run( "ls -al " + oldFile ) ;
      throw "list moved file should be failed" ;
   }
   catch( e )
   {
      if( e !== 2 )
         throw buildException( "checkMove", e, "list " + oldFile, 2, e ) ;
   }
   try
   {
      cmd.run( "ls -al " + newFile ) ;
   }
   catch( e )
   {
      throw buildException( "checkMove", e, "list " + newFile, 0, e ) ;   
   }   
}

/******************************************************************************
*@Description : check copy file
*@author      : Liang XueWang            
******************************************************************************/
function checkCopy( cmd, srcFile, dstFile )
{
   try
   {
      var tmp ;
      tmp = cmd.run( "ls -al " + srcFile + " | awk '{print $1}'" ).split( "\n" ) ;
      var mode1 = tmp[tmp.length-2] ;
      tmp = cmd.run( "ls -al " + dstFile + " | awk '{print $1}'" ).split( "\n" ) ;
      var mode2 = tmp[tmp.length-2]
   }
   catch( e )
   {
      throw buildException( "checkCopy", e, "check " + srcFile + " " + dstFile, 0, e ) ;
   }
   if( mode1 !== mode2 )
   {
      throw buildException( "checkCopy", null, "check mode " + srcFile + " " + dstFile,
                            mode1, mode2 ) ;
   }
}

/******************************************************************************
*@Description : check remove file
*@author      : Liang XueWang            
******************************************************************************/
function checkRemove( cmd, fileName )
{
   try
   {
      cmd.run( "ls -al " + fileName ) ;
      throw "list removed file should be failed" ;
   }
   catch( e )
   {
      if( e !== 2 )
         throw buildException( "checkRemove", e ) ;
   }
}

function main()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   var filename = "/tmp/testFileOperation10833.txt" ;
   var ft1 = new FileTest( localhost, CMSVCNAME ) ;     // 本地File类类型
   var ft2 = new FileTest( localhost, CMSVCNAME, filename ) ;  // 本地file对象
   var ft3 = new FileTest( remotehost, CMSVCNAME ) ;    // 远程File类类型
   var ft4 = new FileTest( remotehost, CMSVCNAME, filename ) ;  // 远程file对象
   
   var fts = [ ft1, ft2, ft3, ft4 ] ;
   
   for( var i = 0; i < fts.length;i++ )
   {
      // 测试创建目录，移动文件，复制文件，删除文件
      fts[i].testFileOperation() ;
      // 测试拷贝目录时指定权限
      fts[i].testCopyWithMode() ;
   }
}

main()
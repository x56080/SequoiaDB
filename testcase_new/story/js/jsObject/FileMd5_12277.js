/******************************************************************************
*@Description : test js object File function: getInfo md5 stat
*               TestLink : 10815 获取File对象信息
*                          10826 File对象查看文件信息
*                          10827 计算File对象文件Md5值
*@auhor       : Liang XueWang
******************************************************************************/
FileTest.prototype.testMd5Mode = function()
{
    this.init() ;
    
    var readOnlyFile = "/tmp/readOnlyFile.txt" ;
    var cannotReadFile = "/tmp/cannotReadFile.txt" ;
    var user = this.cmd.run( "whoami" ).split( "\n" )[0] ;
    
    // create file
    if( this.isLocal )
    {
        var file1 = new File( readOnlyFile ) ;
        file1.write( "123" ) ;
        file1.close() ;
        File.chmod( readOnlyFile, 0444 ) ;
        var file2 = new File( cannotReadFile ) ;
        file2.write( "abc" ) ;
        file2.close() ;
        File.chmod( cannotReadFile, 0222 ) ;
    }
    else
    {
        var file = this.remote.getFile() ;
        var file1 = this.remote.getFile( readOnlyFile ) ;
        file1.write( "123" ) ;
        file1.close() ;
        file.chmod( readOnlyFile, 0444 ) ;
        var file2 = this.remote.getFile( cannotReadFile ) ;
        file2.write( "abc" ) ;
        file2.close() ;
        file.chmod( cannotReadFile, 0222 ) ;
    }
    
    // check read only file md5
    try
    {
        this.file.md5( readOnlyFile ) ;
    }
    catch( e )
    {
        throw buildException( "testMd5Mode", e, "get md5 of file" + readOnlyFile + " " + this, 
                              0, e ) ; 
    }
    
    // check cannot read file 
    try
    {
        this.file.md5( cannotReadFile ) ;
        if( user !== "root" )
        {
            throw 0 ;
        }
    }
    catch( e )
    {
        if( user === "root" )
        {
            throw buildException( "testMd5Mode", null, 
                  "get md5 of file " + cannotReadFile + " with user: " + user + " " + this, 
                  0, e ) ;
        }
        else
        {
            if( e !== -3 )
            {
                throw buildException( "testMd5Mode", e,
                      "get md5 of file " + cannotReadFile + " with user: " + user + " " + this, 
                      -3, e ) ;
            }    
        }
    }
    
    this.cmd.run( "rm -rf " + readOnlyFile ) ;
    this.cmd.run( "rm -rf " + cannotReadFile ) ;
    
    this.release() ;
}

function main()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost() ;
   var remotehost = toolGetRemotehost() ;
   
   var ft1 = new FileTest( localhost, CMSVCNAME ) ;     // 本地File类类型
   var ft2 = new FileTest( remotehost, CMSVCNAME ) ;    // 远程File类类型
   
   var fts = [ ft1, ft2 ] ;
   
   for( var i = 0; i < fts.length;i++ )
   {
      // 测试文件MD5
      fts[i].testMd5Mode() ;
   }
}

main()
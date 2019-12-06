/******************************************************************************
*@Description : test sdblobtool export with different prefer parameter
*@Modify list :
*               2016-07-14   XueWang Liang  Init
******************************************************************************/


function main( db )
{
   initPath() ;
   
   // 检验是否存在sdblobtool工具
   try
   {
      checkLobtool() ;
   }
   catch( e )
   {
      if(e == 127)
         return ;
      throw e ;
   }
  
   var filename = CHANGEDPREFIX + "_test.file" ;	      // CHANGEDPREFIX = "local_test" 文件名
   var exportFile = LocalPath + "/" + filename ;         // 导出文件，导入文件	
   var expFullCL = COMMCSNAME + "." + COMMCLNAME ;       // 导出集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
   var tempFile = LocalPath + "/_temp.file" ;            // 临时文件，保存getlob获得的文件
   
   
   // sdblobtool 导出的选项参数
   var Args = {} ;
   Args[ "hostname" ] = COORDHOSTNAME ;   // 'localhost'
   Args[ "svcname" ] = COORDSVCNAME ;     //  11810 独立模式下为50000
   Args[ "usrname" ] = null ;            
   Args[ "passwd" ] = null ;            
   Args[ "operation" ] = "export" ;        
   Args[ "collection" ] = expFullCL ;       
   Args[ "file" ] = exportFile ;     
   Args[ "prefer" ] = "M" ;               //  优先选择的实例
   Args[ "ssl" ] = false ;                //  使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args[ "ignorefe" ] = false ;           //  大对象已经存在于集合中，忽略
   
   
   choices = [ "m","M","s","S","a","A","1","2","3","4","5","6","7" ] ;
   
   // 创建包含大对象的导出集合
   var lobfile = toolMakeLobfile() ;
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" ) ;
   var lobNum = 1 ;
   var OID = toolPutLobs( expCl, lobfile, lobNum ) ;
   
   // 计算wMd5值
   var wMd5sum = cmd.run( "md5sum " + lobfile ).split( " " ) ;
   var wMd5 = wMd5sum[0] ;
   
   cmd.run( "rm -rf " + tempFile ) ;
   
   for( var i = 0; i < choices.length; ++i)
   {
      try
      {
         Args[ "prefer" ] = choices[ i ] ;
         toolExport( Args ) ;
         // 计算rMd5值
         expCl.getLob( OID[0],tempFile ) ;
         var rMd5sum = cmd.run( "md5sum " + tempFile ).split( " " ) ;
         var rMd5 = rMd5sum[0] ;
         if( rMd5 != wMd5 )
         {
            throw( ">putlob file have md5: " + wMd5 + 
                   " not equal to getlob file md5: " + rMd5 ) ;
         }
      }
      catch( e )
      {
         println( ">fail to test export when prefer is " + Args[ "prefer" ] ) ;
         throw e ;
      }   
      finally
      {
         cmd.run( "rm -rf " + lobfile ) ;
         cmd.run( "rm -rf " + tempFile ) ;
      }
   }
}
   

// Test
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "clean collection in the beginning" ) ;
   main( db ) ;
   cmd.run( "rm -rf sdblobtool.log" ) ;
}
catch( e )
{
   cmd.run( "mkdir -p /tmp/lobtool" ) ;
   cmd.run( "mv sdblobtool.log /tmp/lobtool/exportPrefer.log" ) ;
   throw e ;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
                   "clean collection in the end, wrong" ) ;
   db.close();
}   
   
   
   
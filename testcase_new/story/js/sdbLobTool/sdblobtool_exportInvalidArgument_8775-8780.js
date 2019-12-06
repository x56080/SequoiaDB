/******************************************************************************
*@Description : Invalid Argument test sdblobtool export
*@Modify list :
*               2016-06-20   XueWang Liang  Init
*               覆盖测试用例8775/8776/8777/8778/8779/8780
*******************************************************************************/

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
  
   var filename = CHANGEDPREFIX + "_testexport.file" ;	// CHANGEDPREFIX = "local_test" 文件名
   var exportFile = LocalPath + "/" + filename ;         // 输出文件	
   var expFullCL = COMMCSNAME + "." + COMMCLNAME ;       // 输出集合 COMMCSNAME = CHANGEDPREFIX + "_cs" , COMMCLNAME = CHANGEDPREFIX + "_cl"
   
   // sdblobtool 导出的选项参数
   var Args = {} ;
   Args[ "hostname" ] = COORDHOSTNAME ;     //  'localhost'
   Args[ "svcname" ] = COORDSVCNAME ;       //  11810
   Args[ "usrname" ] = null ;            
   Args[ "passwd" ] = null ;            
   Args[ "operation" ] = "export" ;       
   Args[ "collection" ] = expFullCL ;      
   Args[ "file" ] = exportFile ;      
   Args[ "prefer" ] = "M" ;                 // 优先选择的实例
   Args[ "ssl" ] = false ;                  // 使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   
   // 创建包含大对象的集合COMMCLNAME
   var lobfile = toolMakeLobfile() ;
   var expCl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create CL to export lob" ) ;
   var lobNum = 1 ;
   var OID = toolPutLobs( expCl, lobfile, lobNum ) ;
   cmd.run( "rm -rf " + lobfile ) ;
   
   // 首先生成正确的导出命令并保存到临时变量
   var ExportCmd = toolGetCmdstr( Args ) ;
   var tmp = ExportCmd ;
   
   // 参数非法报错的错误码
   errCode = -6 ;
   
   // 测试导出命令中含有多余不存在的选项 --Illegal
   try
   {
      ExportCmd += " --Illegal something" ;
      cmd.run( ExportCmd ) ;
      println( ">export lob with illegal option, something wrong!") ;
   }
   catch( e )
   {
      var errNumber = getErr( e ) ;
      if( errNumber == errCode )
         println( ">success to test sdblobtool export with illegal option, rc = " + errNumber ) ;
      else
      {
         println( ">fail to test sdblobtool export with illegal option, rc = " + errNumber ) ;
         throw errNumber ;
      }
   }
   finally
   {
      cmd.run( "rm -rf " + exportFile ) ;
      ExportCmd = tmp ;
   }
   
   // 测试导出命令中选项名称拼写错误，如--colection
   try
   {
      ExportCmd = ExportCmd.replace( "--collection", "--colection") ;
      cmd.run( ExportCmd ) ;
      println( ">export lob with wrong option name, something wrong!") ;
   }
   catch( e )
   {
      var errNumber = getErr( e ) ;
      if( errNumber == errCode )
         println( ">success to test sdblobtool export with wrong option name, rc = " + errNumber ) ;
      else
      {
         println( ">fail to test sdblobtool export with wrong option name, rc = " + errNumber ) ;
         throw errNumber ;
      }
   }
   finally
   {
      cmd.run( "rm -rf " + exportFile ) ;
      ExportCmd = tmp ;
   }
   
   
   // 测试命令中只填了选项名称，没填值的情况，如--hostname 空，该选项必须放在最后，放在其他位置是参数不正确错误，参看lobtool_exportErrPara.js
   try
   {
      ExportCmd = ExportCmd.replace( "--hostname "+Args[ "hostname" ], "" ) ;
      ExportCmd += " --hostname" ;
      cmd.run( ExportCmd ) ;
      println( ">export lob with no option value, something wrong!") ;
   }
   catch( e )
   {
      var errNumber = getErr( e ) ;
      if( errNumber == errCode )
         println( ">success to test sdblobtool export with no option value, rc = " + errNumber ) ;
      else
      {
         println( ">fail to test sdblobtool export with no option value, rc = " + errNumber ) ;
         throw errNumber ;
      }
   }
   finally
   {
      cmd.run( "rm -rf " + exportFile ) ;
      ExportCmd = tmp ;
   }
   
   // 测试导出命令中缺少必填项
   var parameter = { "operation":"", "collection":"", "file":"" } ;
   for( var k in parameter )
   {
      try
      {
         ExportCmd = ExportCmd.replace( "--" + k + " " + Args[k], parameter[k] ) ;
         cmd.run( ExportCmd ) ;
         println( ">export lob with no option " + k + ", something wrong!") ;
      }
      catch( e )
      {
         var errNumber = getErr( e ) ;
         if( errNumber == errCode )
            println( ">success to test sdblobtool export with no option " + k + ", rc = " + errNumber ) ;
         else
         {
            println( ">fail to test sdblobtool export with no option " + k + ", rc = " + errNumber ) ;
            throw errNumber ;
         }
      }
      finally
      {
         cmd.run( "rm -rf " + exportFile ) ;
         ExportCmd = tmp ;
      } 
   }
   println( ">success to test sbdlobtool export with Invalid Argument.\n\n" ) ;  
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
   cmd.run( "mv sdblobtool.log /tmp/lobtool/exportInvalidArg.log" ) ;
   throw e ;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
                   "clean collection in the end, wrong" ) ;
   db.close();
}
   
   
   
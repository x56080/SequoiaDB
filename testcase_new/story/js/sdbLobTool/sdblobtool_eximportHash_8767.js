/******************************************************************************
*@Description : Hash split expCollection and impCollection differently
*               test sdblobtool exportfile and importfile md5
*@Modify list :
*               2016-06-24   XueWang Liang  Init
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
  	
  	var filename = CHANGEDPREFIX + "_test.file" ;	      // CHANGEDPREFIX = "local_test" 文件名
   var testFile = LocalPath + "/" + filename ;           // 既是export导出文件,也是getLob读取大对象文件	
   var CsName = CHANGEDPREFIX + "_testHashCS" ;          // 集合空间名
   var expClName = CHANGEDPREFIX + "_exportCl" ;         // 导出集合名
   var expFullCL = CsName + "." + expClName ;            // 导出集合
   var impClName = CHANGEDPREFIX + "_importCl" ;         // 导入集合名
   var impFullCL = CsName + "." + impClName ;            // 导入集合
   
   // sdblobtool 导出导入的选项参数
   var Args = {} ;
   Args[ "hostname" ] = COORDHOSTNAME ;          //   'localhost'
   Args[ "svcname" ] = COORDSVCNAME ;            //   11810
   Args[ "usrname" ] = null ;           
   Args[ "passwd" ] = null ;            
   Args[ "operation" ] = "export" ;        
   Args[ "collection" ] = expFullCL ;       
   Args[ "file" ] = testFile ;       
   Args[ "prefer" ] = "M" ;
   Args[ "ssl" ] = false ;                       //   使用SSL连接，如果不使用SSL连接，shell命令中不应加入该选项参数
   Args[ "ignorefe" ] = false ;                  //   当前大对象已经存在于集合中，忽略这个错误
 
   
   
   // 创建一个包含两个复制组的自动切分域,并在域中创建集合空间和导出集合
   var rg = commGetGroups( db ) ;
   if( rg.length < 3 )
   {
      println( "datagroup num:"+rg.length+" too few!!!\n\n" ) ;
      return ;
   }
   var group = [] ;
   group[0] = rg[0][0].GroupName ;
   group[1] = rg[1][0].GroupName ;
   group[2] = rg[2][0].GroupName ; 
   var lobdomain ;
   var domName = 'lobdomain';
   commDropDomain( db, domName);
   lobdomain = commCreateDomain( db, domName, [ group[0], group[1] ], { AutoSplit:true });
   
   try
   {
      commCreateCS( db, CsName, true, "create testHashCS in the begining", { "Domain":domName} ) ;
      var expCl = commCreateCLByOption( db, CsName, expClName, 
                                { ReplSize:0,"ShardingKey":{"OID":1}, "ShardingType":"hash", "Partition":2048 }, 
                                true, true, true, "create export cl in the begining" ) ;
      
      // 计算wMd5值
      var lobfile = toolMakeLobfile() ;
      var lobNum = 1 ;
      var OID = toolPutLobs( expCl, lobfile, lobNum ) ;
      var wMd5sum = cmd.run( "md5sum " + lobfile ).split( " " ) ;
      var wMd5 = wMd5sum[0] ;
      
      
      // 将大对象导出到文件
      toolExport( Args ) ;
      
      
      // 将域的复制组个数改成三个
      lobdomain.alter( { Groups:[ group[0], group[1], group[2] ] } ) ;
      
      // 新建导入集合，并将导出文件大对象导入该集合中
      var impCl = commCreateCLByOption( db, CsName, impClName, 
                    { ReplSize:0,"ShardingKey":{"OID":1}, "ShardingType":"hash", "Partition":2048 }, 
                    true, false, true, "create import CL in the beginning" ) ;
      Args[ "operation" ] = "import" ;
      Args[ "collection" ] = impFullCL ;
      toolImport( Args ) ;
      cmd.run( "rm -rf " + testFile ) ;
      
      // 检查大对象的条数和OID
      toolCheckLob( impCl,lobNum,OID ) ;
      
      // 使用getLob方法将大对象导出为文件，计算读取Md5值，与写入Md5值比较
      impCl.getLob( OID[0], testFile ) ;
      var rMd5sum = cmd.run( "md5sum " + testFile ).split( " " ) ;
      var rMd5 = rMd5sum[0] ;
      if( rMd5 == wMd5 )
         println( ">success to test eximportHash with rMd5 = wMd5.\n\n") ;
      else
         throw( ">fail to test eximportHash with rMd5 != wMd5.\n\n") ;
   }
   finally
   {
      cmd.run( "rm -rf " + lobfile ) ;
      cmd.run( "rm -rf " + testFile ) ;
      commDropCL( db, CsName, impClName, true, true, "clean import CL in the end" ) ;
      commDropCL( db, CsName, expClName, true, true, "clean export CL in the end" ) ;
      commDropCS( db, CsName, true, "clean the collection space in the end" ) ;
      commDropDomain( db, domName);
   }
     
}

// Test
try
{
   main( db ) ;
   cmd.run( "rm -rf sdblobtool.log" ) ;
}
catch( e )
{
   cmd.run( "mkdir -p /tmp/lobtool" ) ;
   cmd.run( "mv sdblobtool.log /tmp/lobtool/eximportHash.log" ) ;
   throw e ;
}
finally
{
   db.close();
}
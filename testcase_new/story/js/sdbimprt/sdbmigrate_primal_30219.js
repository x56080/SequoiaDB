/******************************************************************************
 * @Description   : seqDB-30219:sdbmigrate导出贴源数据
 * @Author        : Yu Fan
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.02.24
 * @LastEditors   : liuli
 ******************************************************************************/
var csNames = ["cs_30219A", "cs_30219B", "cs_30219C"];
var clNames = ["cl_30219A1", "cl_30219A2", "cl_30219A3"];
var docs = new Array();
var expPrimalResult = new Array();
tmpFileDir += "/30219/";

main( test );
function test ()
{
   // 准备集合、插入记录，并绑定外部模式
   prepareCSCL();

   // 准备导出配置文件
   var exportConf = prepareExportConf();

   // 导出命令
   var command = installDir + "tools/sdbmigrate/bin/sdbexprt.sh" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " --conf " + exportConf + " --primal false" + " --jobs 3 --debug";
   // 执行导出
   cmd.run( command );

   // 删除数据，准备导入
   for( var i = 0; i < csNames.length; i++ )
   {
      var cs = db.getCS( csNames[i] );
      for( var j = 0; j < clNames.length; j++ )
      {
         var cl = cs.getCL( clNames[j] );
         cl.remove();
      }
   }

   // 准备导入配置文件
   var importConf = prepareImportConf();
   // 导入命令
   var command = installDir + "tools/sdbmigrate/bin/sdbimprt.sh" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " --conf " + importConf + " --jobs 3 --debug";
   // 执行导入
   cmd.run( command );

   // 贴源读数据检查结果
   for( var i = 0; i < csNames.length; i++ )
   {
      var cs = db.getCS( csNames[i] );
      for( var j = 0; j < clNames.length; j++ )
      {
         var cl = cs.getCL( clNames[j] );
         var cursor = cl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
         commCompareResults( cursor, docs );
      }
   }

   // 清理环境
   for( var i = 0; i < csNames.length; i++ )
   {
      commDropCS( db, csNames[i], true );
   }
   cmd.run( "rm -rf " + tmpFileDir );
}

function prepareCSCL ()
{
   for( var i = 0; i < csNames.length; i++ )
   {
      commDropCS( db, csNames[i], true );
      db.createCS( csNames[i] );
   }

   var readDefault = 10000;
   for( var i = 0; i < 200; i++ )
   {
      docs.push( { a: i, b: i * 2, c: i * 3 } );
      expPrimalResult.push( { a: i, b: i * 2, c: i * 3, d: readDefault } );
   }

   var schemaDef = {
      a: { Type: "int32" },
      b: { Type: "int32" },
      c: { Type: "int32" },
      d: { Type: "int32", ReadDefault: readDefault }
   };
   for( var i = 0; i < csNames.length; i++ )
   {
      var cs = db.getCS( csNames[i] );
      for( var j = 0; j < clNames.length; j++ )
      {
         var schemaName = csNames[i] + "." + clNames[j];
         commCreateSchema( db, schemaName, schemaDef );
         var cl = cs.createCL( clNames[j], { ReplSize: 0, EnableInfoSchema: true } );
         cl.insert( docs );
         cl.addSchema( schemaName );
      }
   }
}

function prepareExportConf ()
{
   cmd.run( "rm -rf " + tmpFileDir );
   cmd.run( "mkdir -p " + tmpFileDir );
   var filename = tmpFileDir + "export30219.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=3\n" );
   for( var i = 1; i <= clNames.length; i++ )
   {
      file.write( "[collection" + i + "]\n" );
      file.write( "name=" + csNames[0] + "." + clNames[i - 1] + "\n" );
      file.write( "type=json\n" );
      file.write( "dir=" + tmpFileDir + "\n" );
   }

   file.write( "[collectionspaces]\n" );
   file.write( "number=2\n" );
   for( var i = 1; i < csNames.length; i++ )
   {
      cmd.run( "mkdir -p " + tmpFileDir + csNames[i] );
      file.write( "[collectionspace" + i + "]\n" );
      file.write( "name=" + csNames[i] + "\n" );
      file.write( "type=json\n" );
      file.write( "dir=" + tmpFileDir + csNames[i] + "\n" );
   }
   return filename;
}

function prepareImportConf ()
{
   var filename = tmpFileDir + "import30219.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=3\n" );
   for( var i = 1; i <= clNames.length; i++ )
   {
      file.write( "[collection" + i + "]\n" );
      file.write( "name=" + csNames[0] + "." + clNames[i - 1] + "\n" );
      file.write( "type=json\n" );
      file.write( "file=" + tmpFileDir + csNames[0] + "." + clNames[i - 1] + "." + "0.json\n" );
   }

   file.write( "[collectionspaces]\n" );
   file.write( "number=2\n" );
   for( var i = 1; i <= csNames.length; i++ )
   {
      file.write( "[collectionspace" + i + "]\n" );
      file.write( "name=" + csNames[i] + "\n" );
      file.write( "type=json\n" );
      file.write( "dir=" + tmpFileDir + csNames[i] + "\n" );
   }
   return filename;
}
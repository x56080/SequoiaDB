/******************************************************************************
 * @Description   : seqDB-30218 :sdbexprt导出贴源数据
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.24
 * @LastEditTime  : 2023.03.06
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
var csName = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt30218";
var clname2 = COMMCLNAME + "_sdbimprt30128_1";
var clname3 = COMMCLNAME + "_sdbimprt30128_2";
var docs = [];
var expResult = [];
var expPrimalResult = [];

main( test );
function test ()
{
   prepareCSCL();
   testExprtImprtJson();

   // 贴源读数据检查结果
   var cs = db.getCS( csName );
   var cl2 = cs.getCL( clname2 );
   var cl3 = cs.getCL( clname3 );

   // 校验数据
   var actResult = cl2.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = cl3.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
   commCompareResults( actResult, expPrimalResult );
   println( "actResult:" + actResult );
   println( "expResult:" + JSON.stringify( expResult ) );
   println( "expPrimalResult:" + JSON.stringify( expPrimalResult ) );
}

function prepareCSCL ()
{
   commDropCS( db, csName, true );
   db.createCS( csName );

   var schemaDef = {
      a: { Type: "int32" },
      b: { Type: "int32" }
   };

   var cs = db.getCS( csName )
   var schemaName = csName + "." + clname;
   commDropSchema( db, schemaName, true );
   var schema = commCreateSchema( db, schemaName, schemaDef );
   var cl = cs.createCL( clname, { ReplSize: 0, EnableInfoSchema: true } );

   cl.addSchema( schemaName );

   for( var i = 0; i < 200; i++ )
   {
      docs.push( { a: i, b: i * 2 } );
      expResult.push( { a: i, b: i * 2, c: 10 } );
      expPrimalResult.push( { a: i, b: i * 2 } );
   }
   cl.insert( docs );

   schema.addColumn( "c", { Type: "int32", ReadDefault: 10 } );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt30128.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csName +
      " -l " + clname +
      " --type json" +
      " --fields a,b,c " +
      "--primal=false" +
      " --file " + jsonfile;
   testRunCommand( command );

   commCreateCL( db, csName, clname2 );
   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csName +
      " -l " + clname2 +
      " --type json" +
      " --file " + jsonfile;
   testRunCommand( command );

   cmd.run( "rm -rf " + jsonfile );

   var jsonfile = tmpFileDir + "sdbexprt30128.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csName +
      " -l " + clname +
      " --type json" +
      " --fields a,b,c " +
      "--primal=true" +
      " --file " + jsonfile;
   testRunCommand( command );

   commCreateCL( db, csName, clname3 );
   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csName +
      " -l " + clname3 +
      " --type json" +
      " --file " + jsonfile;
   testRunCommand( command );

   cmd.run( "rm -rf " + jsonfile );

   var jsonfile = tmpFileDir + "sdbexprt30128.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csName +
      " -l " + clname +
      " --type json" +
      " --fields a,b,c " +
      "--primal=1" +
      " --file " + jsonfile;

   // assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   // {
   //    testRunCommand( command );
   // } )
   cmd.run( "rm -rf " + jsonfile );
}

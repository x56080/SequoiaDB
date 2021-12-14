/******************************************************************************
 * @Description   : seqDB-24707:unloadCS，执行find操作
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.13
 * @LastEditTime  : 2021.12.14
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_cs24707";
testConf.clName = COMMCSNAME + "_cl24707";

main( test );
function test ( args )
{
   var varCL = args.testCL
   var doc = [];
   doc.push( { a: 1 } );
   varCL.insert( doc );
   var x = 0;
   db.unloadCS( testConf.csName );

   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      varCL.find().next();
   } );

   db.loadCS( testConf.csName );
   var cursor = varCL.find();
   commCompareResults( cursor, doc );
}
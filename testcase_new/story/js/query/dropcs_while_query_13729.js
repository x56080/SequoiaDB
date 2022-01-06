/******************************************************************************
 * @Description   : seqDB-13729:游标未关闭时，在同一个session中删除cs
 * @Author        : xiaojunHu
 * @CreateTime    : 2014.09.26
 * @LastEditTime  : 2021.12.10
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/

testConf.csName = COMMCSNAME + "_13729";
testConf.clName = COMMCLNAME + "_13729";

main( test );
function test ( testPara )
{
   var rd = new commDataGenerator();
   var recs = rd.getRecords( 1000, "string", ['a', 'b', 'c'] );
   testPara.testCL.insert( recs );

   //cursor not close
   try
   {
      var rc = testPara.testCL.find();
      rc.next();
      dropcsSameSession( testConf.csName );
   }
   finally
   {
      rc.close();
   }
}

function dropcsSameSession ( csName )
{
   db.dropCS( csName );
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( csName );
   } );
}
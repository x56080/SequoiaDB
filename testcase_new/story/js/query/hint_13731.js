/*******************************************************************************
*@Description : seqDB-13731:查询指定hint：
                          强制走表扫描、hint走不存在的索引
                          强制走原本要走的索引、强制走另外一个索引
*@Modify List : 2014-06-12   xiaojun Hu   Init
                2016-03-17   Ting YU      Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_cl_13731";
main( test );

function test (testPara)
{
   var idxName1 = "_a_idx";
   var idxName2 = "_b_idx";
   testPara.testCL.createIndex( idxName1, { a: -1 } );
   testPara.testCL.createIndex( idxName2, { b: 1 } );
   
   var recs = [];
   for( var i = 0; i < 100; i++ ) 
   {
      recs.push( { a: i, b: i + 0.95 } );
   }
   testPara.testCL.insert( recs );
   
   println( "---begin to query by hint({'':null}}" );
   var rc = testPara.testCL.find( { a: { $gte: 0 } } ).sort( { a: 1 } ).hint( { "": null } );
   checkExplain( rc, "" );
   checkRec( rc, recs );

   println( "---begin to query by hint({'':null}}" );
   var rc = testPara.testCL.find( { a: { $gte: 0 } } ).sort( { a: 1 } ).hint( { "": null } );
   checkExplain( rc, "" );
   checkRec( rc, recs );

   println( "---begin to query, hinted by non-existed index" );
   var rc = testPara.testCL.find().sort( { b: 1 } ).hint( { "": "non_existed_index" } );
   checkExplain( rc, idxName2 );
   checkRec( rc, recs );

   println( "---begin to query, hinted by index" );
   var rc = testPara.testCL.find().sort( { a: 1 } ).hint( { "": idxName1 } );
   checkExplain( rc, idxName1 );
   checkRec( rc, recs );

   println( "---begin to query, hinted by another index" );
   var rc = testPara.testCL.find().sort( { b: 1 } ).hint( { "": idxName1 } );
   checkExplain( rc, idxName1 );
   checkRec( rc, recs );  
}
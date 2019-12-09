/************************************
*@Description: seqDB-18433:集合中无记录，存在多个索引，匹配索引字段数多的访问计划
*@author:      zhaoyu
*@createdate:  2019.6.12
*@testlinkCase: seqDB-18433
**************************************/
function main ()
{
   var clName = COMMCLNAME + "18433";
   commDropCL( db, COMMCSNAME, clName, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.createIndex( "a", { a: 1 } );
   dbcl.createIndex( "ab", { a: 1, b: 1 } );
   dbcl.createIndex( "abc", { a: 1, b: 1, c: 1 } );

   var explainIdx = dbcl.find( { a: 1 } ).explain().next().toObj().IndexName;
   checkExplain( explainIdx, "a" );

   explainIdx = dbcl.find( { a: 1, b: 1 } ).explain().next().toObj().IndexName;
   checkExplain( "ab", "ab" );

   explainIdx = dbcl.find( { a: 1, b: 1, c: 1 } ).explain().next().toObj().IndexName;
   checkExplain( explainIdx, "abc" );

   var doc = [];
   for( var i = 0; i < 10000; i++ )
   {
      doc.push( { a: i, b: i, c: i, d: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" + i } )
   }
   dbcl.insert( doc );

   var explainIdx = dbcl.find( { a: 1 } ).explain().next().toObj().IndexName;
   checkExplain( explainIdx, "a" );

   explainIdx = dbcl.find( { a: 1, b: 1 } ).explain().next().toObj().IndexName;
   checkExplain( "ab", "ab" );

   explainIdx = dbcl.find( { a: 1, b: 1, c: 1 } ).explain().next().toObj().IndexName;
   checkExplain( explainIdx, "abc" );

   commDropCL( db, COMMCSNAME, clName );
}

main();

function checkExplain ( expectExplain, actualExplain )
{
   if( expectExplain != actualExplain )
   {
      throw buildException( "checkExplain", "Error", checkExplain, expectExplain, actualExplain );
   }
}

/************************************************************************
*@Description: seqDB-1974:$aggregate使用数组查询
*@Author:  2015/10/21  huangxiaoni
************************************************************************/
main();

function main ()
{
   var clName = COMMCLNAME + "_1974";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   cl.insert( { a: { b: [1, 2] } } );

   var expResult = [ { "b": 1 } ];
   var cursor = cl.aggregate( { $project: { b: "$a.b.$[0]" } } );
   commCompareResults ( cursor, expResult );
   
   commDropCL( db, COMMCSNAME, clName, false, false );
}

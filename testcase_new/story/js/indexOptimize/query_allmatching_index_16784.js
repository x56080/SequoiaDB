/************************************
*@Description: 查询字段匹配所有索引字段  
*@author:      liuxiaoxuan
*@createdate:  2018.12.12
*@testlinkCase: seqDB-16784
**************************************/
function main()
{
   //create CL
   var clName = COMMCLNAME + "_index_16784";
   commDropCL(db, COMMCSNAME, clName, true, true);

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   // insert data , 32 fields
   var rd = new commDataGenerator();
   for(var i = 0; i < 5; i++){
      var objs = rd.getRecords( 10000, ["int", "int", "int", "int", "int", "int", "int","int", "int", "int", "int", "int", "int", "int","int", 
                                        "int", "int", "int", "int", "int", "int","int", "int", "int", "int", "int", "int", "int", "int", "int",
                                        "int", "int"], ['a0','b0','c0','d0','e0','f0','h0','g0','i0','j0','k0','l0','m0','n0','o0','p0',
                                        'a1','b1','c1','d1','e1','f1','h1','g1','i1','j1','k1','l1','m1','n1','o1','p1'] );
      dbcl.insert(objs);
   }

   // create index
   commCreateIndex(dbcl, "idx1", {a0:1, b0:-1, c0:-1, d0:1, e0:-1, f0:1, h0:-1, g0:1, i0:1, j0:1, k0:-1, l0:1, m0:-1, n0:1, o0:1 , p0:-1,
                                  a1:1, b1:-1, c1:-1, d1:1, e1:-1, f1:1, h1:-1, g1:1, i1:1, j1:1, k1:-1, l1:1, m1:-1, n1:1, o1:1 , p1:-1});
 
   var findCond = {c0: {$gt: 1000}, a0: {$gt: 10000}, b0: {$lt: 10000}, h0: {$gt: 1000}, j0: {$gt: 10000}, f0: {$lt: 10000},
                   e0: {$gt: 1000}, d0: {$gt: 10000}, g0: {$lt: 10000}, i0: {$gt: 1000}, m0: {$gt: 10000}, k0: {$lt: 10000},
                   n0: {$gt: 1000}, l0: {$gt: 10000}, p0: {$lt: 10000}, o0: {$gt: 1000}, c1: {$gt: 1000}, a1: {$gt: 10000},
                   b1: {$lt: 10000}, h1: {$gt: 1000}, j1: {$gt: 10000}, f1: {$lt: 1000}, e1: {$gt: 1000}, d1: {$gt: 10000},
                   g1: {$lt: 10000}, i1: {$gt: 1000}, m1: {$gt: 10000}, k1: {$lt: 10000}, n1: {$gt: 1000}, l1: {$gt: 10000},
                   p1: {$lt: 10000}, o1: {$gt: 1000}};
   var actResults = getExplain(dbcl, findCond);
   checkExplain(actResults, "ixscan", "idx1");
 
   commDropCL(db, COMMCSNAME, clName, true, true);
}

main();

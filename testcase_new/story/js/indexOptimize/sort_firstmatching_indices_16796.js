/************************************
*@Description: 多个索引与查询条件匹配字段相等，使用索引字段和选择排序匹配字段都相等     
*@author:      liuxiaoxuan
*@createdate:  2018.12.12
*@testlinkCase: seqDB-16796
**************************************/
function main()
{
   //create CL
   var clName = COMMCLNAME + "_index_16796";
   commDropCL(db, COMMCSNAME, clName, true, true);

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   // insert data  
   var rd = new commDataGenerator();
   for(var i = 0; i < 5; i++){
      var objs = rd.getRecords( 10000, ["int", "int", "string", "string", "string", "string", "string"], ['a','b','c','d','f','g','h'] );
      dbcl.insert(objs);
   }

   // create index
   commCreateIndex(dbcl, "abcd", {a:-1, b:-1, c:-1, d:-1, h:1});
   commCreateIndex(dbcl, "abcfh", {b:-1, a:-1, c:-1, f:-1, h:1});
   commCreateIndex(dbcl, "abcgh", {a:1, b:1, c:-1, g:1, h:-1});

   // query rule equal, index rule equal, choose the first index 
   var findCond = {c: {$gt: ""}, a: {$gt: 10000}, b: {$lt: 10000}};
   var sortCond = {a:-1, b:-1};
   var actResults = getExplain(dbcl, findCond, sortCond);
   checkExplain(actResults, "ixscan", "abcd");

   commDropCL(db, COMMCSNAME, clName, true, true);
}

main();

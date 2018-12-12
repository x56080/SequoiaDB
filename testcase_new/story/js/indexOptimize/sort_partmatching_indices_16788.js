/************************************
*@Description: 多个索引与查询条件匹配字段相等，指定sort字段部分匹配索引   
*@author:      liuxiaoxuan
*@createdate:  2018.12.12
*@testlinkCase: seqDB-16788
**************************************/
function main()
{
   //create CL
   var clName = COMMCLNAME + "_index_16788";
   commDropCL(db, COMMCSNAME, clName, true, true);

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   // insert data  
   var rd = new commDataGenerator();
   for(var i = 0; i < 5; i++){
      var objs = rd.getRecords( 10000, ["int", "int", "string", "string", "string"], ['a','b','c','d','f'] );
      dbcl.insert(objs);
   }

   // create index
   commCreateIndex(dbcl, "abc", {a:1, b:-1, c:-1});
   commCreateIndex(dbcl, "abd", {a:1, b:1, d:-1});
   commCreateIndex(dbcl, "abcf", {a:-1, b:1, d:-1, f:1});
 
   // query fields match many index fields
   var findCond = {b: {$gt: 1000}, d: {$gt: ""}, a: {$gt: 1000}};
   var sortCond = {a: 1, b: -1, d:1};
   var actResults = getExplain(dbcl, findCond, sortCond);
   checkExplain(actResults, "ixscan", "abcf");

   // query fields match one index field
   var findCond = {a: {$gt: ""}};
   var sortCond = {a: -1, b: 1};
   var actResults = getExplain(dbcl, findCond, sortCond);
   checkExplain(actResults, "ixscan", "abc");
 
   commDropCL(db, COMMCSNAME, clName, true, true);
}

main();

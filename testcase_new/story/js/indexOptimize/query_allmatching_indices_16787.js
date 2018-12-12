/************************************
*@Description: 多个索引与查询条件全匹配   
*@author:      liuxiaoxuan
*@createdate:  2018.12.12
*@testlinkCase: seqDB-16787
**************************************/
function main()
{
   //create CL
   var clName = COMMCLNAME + "_index_16787";
   commDropCL(db, COMMCSNAME, clName, true, true);

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   // insert data  
   var rd = new commDataGenerator();
   for(var i = 0; i < 5; i++){
      var objs = rd.getRecords( 10000, ["int", "int", "string", "string", "string", "string"], ['a','b','c','d','f','g'] );
      dbcl.insert(objs);
   }

   // create index
   commCreateIndex(dbcl, "abcd", {a:-1, b:1, c:-1, d:-1});
   commCreateIndex(dbcl, "abcf", {a:-1, b:1, c:-1, f:-1});
   commCreateIndex(dbcl, "abcfg", {a:1, b:1, c:-1, f:1, g:-1});
 
   // query field positive order
   var findCond = {a: {$gt: 10000}, b: {$lt: 10000}, c: {$gt: ""}, f: {$gt: ""}};
   var actResults = getExplain(dbcl, findCond);
   checkExplain(actResults, "ixscan", "abcf");

   // query field disorder
   var findCond = {f: {$gt: ""}, a: {$gt: 10000}, g: {$gt: ""}, c: {$gt: ""}, b: {$lt: 10000}};
   var actResults = getExplain(dbcl, findCond);
   checkExplain(actResults, "ixscan", "abcfg");
 
   commDropCL(db, COMMCSNAME, clName, true, true);
}

main();

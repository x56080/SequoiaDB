/************************************
*@Description: 查询字段不匹配索引，带sort匹配索引字段   
*@author:      liuxiaoxuan
*@createdate:  2018.12.12
*@testlinkCase: seqDB-16785
**************************************/
function main()
{
   //create CL
   var clName = COMMCLNAME + "_index_16785";
   commDropCL(db, COMMCSNAME, clName, true, true);

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   // insert data  
   var rd = new commDataGenerator();
   for(var i = 0; i < 5; i++){
      var objs = rd.getRecords( 10000, ["int", "int", "string", "string"], ['a','b','c','d'] );
      dbcl.insert(objs);
   }

   // create index
   commCreateIndex(dbcl, "abc", {a:1, b:1, c:-1});
 
   var findCond = {c: {$gt: 10000}};
   var sortCond = {a: 1, b: 1};
   var actResults = getExplain(dbcl, findCond, sortCond);
   checkExplain(actResults, "ixscan", "abc");
 
   commDropCL(db, COMMCSNAME, clName, true, true);
}

main();

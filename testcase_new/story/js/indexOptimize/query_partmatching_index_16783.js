/************************************
*@Description: 查询字段包含部分索引字段，匹配索引  
*@author:      liuxiaoxuan
*@createdate:  2018.12.12
*@testlinkCase: seqDB-16783
**************************************/
main();
function main()
{
   //create CL
   var clName = COMMCLNAME + "_index_16783";
   commDropCL(db, COMMCSNAME, clName, true, true);

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   
   // insert data  
   var rd = new commDataGenerator();
   for(var i = 0; i < 5; i++){
      var objs = rd.getRecords( 10000, ["int", "int", "string", "string"], ['a','b','c','d'] );
      dbcl.insert(objs);
   }

   // create index
   commCreateIndex(dbcl, "abcd", {a:1, b:1, c:-1, d:1});
 
   var findCond = {c: {$gt: ""}, a: {$gt: 10000}, b: {$lt: 10000}};
   var actResults = getExplain(dbcl, findCond);
   checkExplain(actResults, "ixscan", "abcd");
 
   commDropCL(db, COMMCSNAME, clName, true, true);
}

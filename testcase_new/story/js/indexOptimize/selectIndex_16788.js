/************************************
*@Description: seqDB-16788 multiple indexes match fields for query conditions,
               sort field matching partial index.
*@author:      wuyan
*@date:        2018.12.12
**************************************/
main();
function main()
{ 
   var clName = COMMCLNAME + "_selectIndex_16788";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );      

   var rd = new commDataGenerator();
   var objs = rd.getRecords( 10000, ["string", "int", "date", "long", "string", "float"], ['stra','intb','datec','ld','stre','f'] );
   dbcl.insert(objs);   

   var indexName1 = "indexa";
   var indexName2 = "indexb";
   var indexName3 = "indexc";
   commCreateIndex( dbcl, indexName1, {'stra':1,'intb':1,'datec':1,'f':1} );
   commCreateIndex( dbcl, indexName2, {'stra':-1,'intb':1,'ld':-1,'f':1} );
   commCreateIndex( dbcl, indexName3, {'stra':-1,'intb':-1,'ld':-1,'datec':1} );
   
   println("---test a:query matches multiple fields.");
   var findCond = {'stra':{$lt:19000},'intb':{$gt:0}};
   var sortCond = {'stra':1,'intb':-1,'ld':1};
   var getExplainReslut = getExplain( dbcl, findCond, sortCond );
   checkExplain( getExplainReslut, "ixscan", indexName2 );
   
   println("---test b:query matches one field.");
   var findCond1 = {'stra':{$gt:0}};
   var sortCond1 = {'stra':-1,'intb':-1,'ld':-1};
   var getExplainReslut1 = getExplain( dbcl, findCond1, sortCond1 );
   checkExplain( getExplainReslut1, "ixscan", indexName3 );
   commDropCL(db, COMMCSNAME, clName, true, true);
}


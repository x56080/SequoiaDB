/************************************
*@Description: seqDB-16789 multiple indexes match fields for query conditions,
               all sort fields match index.
*@author:      wuyan
*@date:        2018.12.12
**************************************/
main();
function main()
{ 
   var clName = COMMCLNAME + "_selectIndex_16789";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );      

   var rd = new commDataGenerator();
   var objs = rd.getRecords( 10000, ["string", "int", "date", "long", "string", "float"], ['stra','intb','datec','ld','stre','f'] );
   dbcl.insert(objs);   

   var indexName1 = "indexa";
   var indexName2 = "indexb";
   commCreateIndex( dbcl, indexName1, {'stra':-1,'intb':1,'datec':1,'f':1} );
   commCreateIndex( dbcl, indexName2, {'stra':1,'intb':1,'ld':-1,'f':1} );
   
   println("---test a:sort fields and index fields in the same order.");
   var findCond = {'stra':{$lt:1000},'intb':{$gt:0}};
   var sortCond = {'stra':1,'intb':1,'ld':1,'f':1};
   var getExplainReslut = getExplain( dbcl, findCond, sortCond );
   checkExplain( getExplainReslut, "ixscan", indexName2 );
   
   println("---test b:sort fields and index fields in different order.");
   var findCond1 = {'stra':{$gt:1000},'intb':{$gt:0}};
   var sortCond1 = {'f':1,'stra':1,'ld':1,'intb':-1};;
   var getExplainReslut1 = getExplain( dbcl, findCond1, sortCond1 );
   checkExplain( getExplainReslut1, "ixscan", indexName1 );
   commDropCL(db, COMMCSNAME, clName, true, true);
}


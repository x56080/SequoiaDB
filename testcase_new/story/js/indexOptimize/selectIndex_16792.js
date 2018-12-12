/************************************
*@Description: seqDB-16792 multiple indexes match fields for query conditions,
               the query condition contains all index fields.
*@author:      wuyan
*@date:        2018.12.12
**************************************/
main();
function main()
{ 
   var clName = COMMCLNAME + "_selectIndex_16792";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );      

   var rd = new commDataGenerator();
   var objs = rd.getRecords( 10000, [ "long", "string", "int", "date","string", "float"], ['la','strb','intc','dated','stre','f'] );
   dbcl.insert(objs);   

   var indexName1 = "indexa";
   var indexName2 = "indexb";
   var indexName3 = "indexc";
   var indexName4 = "indexd";
   commCreateIndex( dbcl, indexName1, {'la':1} );
   commCreateIndex( dbcl, indexName2, {'la':1,'strb':-1} );
   commCreateIndex( dbcl, indexName3, {'la':1,'strb':-1,'intc':1} );
   commCreateIndex( dbcl, indexName4, {'la':1,'strb':-1,'intc':1,'dated':1} );
   
   println("---test a:one query fields.");
   var findCond1 = {'la':{$lt:1000}};   
   var getExplainReslut1 = getExplain( dbcl, findCond1 );
   checkExplain( getExplainReslut1, "ixscan", indexName1 );
   
   println("---test b:two query fields.");
   var findCond2 = {'strb':{$gt:0},'la':{$lt:1000}};   
   var getExplainReslut2 = getExplain( dbcl, findCond2 );
   checkExplain( getExplainReslut2, "ixscan", indexName2 );
   
   println("---test c:three query fields.");
   var findCond3 = {'strb':{$gt:0},'intc':{$ne:1000},'la':{$lt:1000}};   
   var getExplainReslut3= getExplain( dbcl, findCond3 );
   checkExplain( getExplainReslut3, "ixscan", indexName3 );
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}


/************************************
*@Description: seqDB-16793 query condition matching partial index,
               and matches the same number of index fields.
*@author:      wuyan
*@date:        2018.12.12
**************************************/
main();
function main()
{ 
   var clName = COMMCLNAME + "_selectIndex_16793";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );      

   var rd = new commDataGenerator();
   var objs = rd.getRecords( 10000, [ "long", "string", "int", "date","string", "float"], ['la','strb','intc','dated','stre','f'] );
   dbcl.insert(objs);   

   var indexName1 = "indexa1";
   var indexName2 = "indexb1";   
   commCreateIndex( dbcl, indexName1, {'la':1,'strb':1,'intc':1,'dated':1,'stre':1} );
   commCreateIndex( dbcl, indexName2, {'la':-1,'strb':1,'intc':1,'f':-1} );     
   
   var findCond = {'strb':{$gt:0},'intc':1,'la':{$lt:1000}};   
   var getExplainReslut1 = getExplain( dbcl, findCond );
   checkExplain( getExplainReslut1, "ixscan", indexName2 );   
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}


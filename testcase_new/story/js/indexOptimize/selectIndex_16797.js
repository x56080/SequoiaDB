/************************************
*@Description: seqDB-16797 create index of the same index fields ,multiple indexes match fields for query conditions.
*@author:      wuyan
*@date:        2018.12.12
**************************************/
main();
function main()
{ 
   var clName = COMMCLNAME + "_selectIndex_16797";
   commDropCL(db, COMMCSNAME, clName, true, true);
   var dbcl = commCreateCL( db, COMMCSNAME, clName );      

   var rd = new commDataGenerator();
   var objs = rd.getRecords( 10000, [ "string", "int", "date","long", "string", "float"], ['stra','intb','datec','ld','stre','f'] );
   dbcl.insert(objs);   

   var indexName1 = "indexa";
   var indexName2 = "indexb";    
   commCreateIndex( dbcl, indexName1, {'stra':-1,'datec':1,'stre':1} );
   commCreateIndex( dbcl, indexName2, {'datec':1,'stra':-1,'stre':1} );      
   
   var findCond = {'datec':{$ne:''},'stra':{$lt:1000}};   
   var getExplainReslut = getExplain( dbcl, findCond );
   checkExplain( getExplainReslut, "ixscan", indexName1 );     
   
   commDropCL(db, COMMCSNAME, clName, true, true);
}


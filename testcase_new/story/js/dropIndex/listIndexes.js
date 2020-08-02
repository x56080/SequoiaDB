/****************************************************************************
@Description : List all indexes. List indexes and have index name.
@Modify list :
2014-5-16  xiaojun Hu  create
****************************************************************************/
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "Failed to clear the collectionspace first :" + e );
   throw e;
}

try
{
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME );
}
catch( e )
{
   println( "Failed to create CS and CL, rc=" + e );
   throw e;
}

//index key :"no". general index
try
{
   cl.createIndex( "noIndex", { no: 1 }, false, false );
   inspecIndex( cl, "noIndex", "no", 1, false, false );
}
catch( e )
{
   println( "Failed to create index noIndex, rc= " + e );
   throw e;
}

//index key : "name". unique index
try
{
   cl.createIndex( "nameIndex", { "name": -1 }, true, false );
   inspecIndex( cl, "nameIndex", "name", -1, true, false );
}
catch( e )
{
   println( "Failed to create index nameIndex, rc= " + e );
   throw e;
}

//index key : "姓名". enforced unique index
try
{
   cl.createIndex( "姓名索引", { "姓名": 1 }, true, true );
   inspecIndex( cl, "姓名索引", "姓名", 1, true, true );
}
catch( e )
{
   println( "Failed to create index '姓名索引', rc= " + e );
   throw e;
}

//insert data to cl after create index
/*try
{
cl.insert( {no:001, name:"A", "姓名":"秦", age:1} ); 
cl.insert( {no:002, name:"B", "姓名":"怪", age:2} ); 
cl.insert( {no:003, name:"C", "姓名":"张"} ); 
cl.insert( {no:004, "姓名":"庄"} ); 
var count = cl.count(); 
if( 4 != count )
{
println( "Wrong number of record, count=" + count ); 
throw "ErrNumRecord"; 
}
}
catch( e )
{
println( "Failed to insert data after create index, rc=" + e ); 
throw e; 
}
*/
//1.list indexes without index name.
try
{
   var listIdx = cl.listIndexes();
   listIdx = listIdx.toString();
   var listIdxN = cl.listIndexes( "nameIndex" );
   listIdxN = listIdxN.toString();
   if( listIdx != listIdxN )
   {
      println( "Wrong result of list indexes" );
      throw "ErrlistIndex";
   }
}
catch( e )
{
   println( "Failed to list Indexes, rc=" + e );
   throw e;
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "Failed to drop CS end of program, rc=" + e );
   throw e;
}



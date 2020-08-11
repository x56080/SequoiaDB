/*************************************************************************************
@Description : get the index that we created.
@Modify list :
2014-5-16  xiaojun Hu  create
*************************************************************************************/
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

//clear the environment end
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "Failed to drop CS int the end, rc=" + e );
   throw e;
}


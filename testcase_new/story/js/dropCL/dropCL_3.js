//drop collection
//innomal case2
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" ); 
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e ); 
   throw e; 
}

try
{
   
   //var varCS = db.createCS( COMMCSNAME ); 
   
   //var varCL = varCS.createCL( COMMCLNAME, {ReplSize:0} ); 
   
}
catch( e )
{
   throw e; 
}

try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" ); 
}
catch( e )
{
   println( "failed to create cs, rc=" + e ); 
   throw e; 
}


try
{
   varCS.dropCL( "bar.cs" ); 
}
catch( e )
{
   if( e != SDB_INVALIDARG && e != SDB_DMS_NOTEXIST )
   {
      println( " error is not " + SDB_INVALIDARG + " \n or error is not " + SDB_DMS_NOTEXIST ); 
      throw e; 
   }
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, 
   "drop colleciton in the end" ); 
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e ); 
   throw e; 
}

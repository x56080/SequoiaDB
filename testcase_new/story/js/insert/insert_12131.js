// insert record.
// normal case.
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" ) ;
}
catch(e)
{
   println( "unexpected err happened when clear cs:" + e ) ;
   throw e ;
}

try{
var claSize = new RSize( COMMCSNAME );
var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
var varCL = varCS.createCL(COMMCLNAME,{ReplSize:claSize.ReplSize(),Compressed:true});
}catch( e ){
   throw e ;	
}
 var insert_string = {"!@#%^&":"&*()?><"} ; 

try
{
   varCL.insert(insert_string) ;
}
catch ( e )
{
   println( "failed to insert record, rc= " + e ) ;
   throw e ;
}


try
{
   if (varCL.count() != 1)
      throw "number error";
   robj = varCL.find().next().toObj() ;
   if (!compareObj(insert_string, robj))
      throw "compare failed";
}
catch ( e )
{
   println( "failed to read record, rc= " + e ) ;
   throw e ;
}
// rc = rc.toArray();
// if( 1 != rc.length ){
//    throw -1 ; 	
// } 

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the end" ) ;
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}

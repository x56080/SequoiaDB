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
   var varCL = varCS.createCL(COMMCLNAME,{ReplSize:claSize.ReplSize(),
                                          Compressed:true});
}catch( e ){
   throw e ;
}

var jx = 0 ;
function str_name()
{
   if( jx != 9 ){
      jx++;
      return ( { a: str_name()} )
   }else{
     return {a:100};
   }

}

var insert_string = str_name();
try
{
   varCL.insert(insert_string) ;
}
catch ( e )
{
  throw "insert a big bson fail";
}

try
{
   if (varCL.count() !=1)
      throw "number error";
   robj = varCL.find().next().toObj(); 
   if(!compareObj(insert_string, robj))
      throw "compare failed";
}
catch (e)
{
   println("verify failed, err:" + e);
   throw e;
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop colleciton in the end" );
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}

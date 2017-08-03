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
var varCL = varCS.createCL(COMMCLNAME,{ReplSize:claSize.ReplSize()},{Compressed:true});
}catch( e ){
   throw e ;
}

var str_1 = "" ; 
for(var i = 0 ; i < 1000 ; ++i ){
   
   str_1 = str_1+"che" ; 
   	
}

var str_2 = "{name:\"qiu\",balance:1.2}" ; 
for(var i = 0 ; i < 1000 ; ++i ){
   
   str_2 = str_2+",{name:\"qiu\",balance:1.2}" ; 
   	
}


var insert_string = {str:str_1 , array:[str_2]} ; 

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
      throw "the amount of record is not correct";
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
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "drop colleciton in the end" );
}
catch ( e )
{
   println( "failed to drop cs, rc= " + e ) ;
   throw e ;
}

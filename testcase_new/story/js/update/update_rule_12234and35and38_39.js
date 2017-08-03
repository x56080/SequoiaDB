// update record.
// unnormal rule. 
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
   println( "Create collection: " + COMMCLNAME + "failed: " + e ) ;
   throw e ;
}

try
{
   varCL.insert({a:[1,2],salary:100}) ;
}
catch ( e )
{
   println( "failed to insert record, rc= " + e ) ;
   throw e ;
}
var res = false ;
try
{
   varCL.update( {$addtoset:{b:2}}) ;
}
catch ( e )
{ 
	 if ( e == -6 )
	 {
        res = true ;
	 }
	 else
	 {
	    println( "Update {$addtoset:{b:2}} failed: " + e ) ;
	 }
}
if ( !res )
{
  throw -1	
}

var res1 = true ;
try
{
   varCL.update( {$pull:{"a.0":1}}) ;
}
catch ( e )
{
   res1 = false ;
   println( "Update {$pull:{a.0:1}} failed: " + e ) ;
}
if ( !res1 )
{
  throw -1	
}

var res2 = true ;
try
{
   varCL.update( {$push:{salary:1}}) ;
}
catch ( e )
{ 
   res2 = false ;
   println( "Update {$push:{salary:1}} failed: " + e ) ;
}
if ( !res2 )
{
  throw -1	
}
var res3 = false ;
try
{
   varCL.update( {$pull_all:{a:3}}) ;
}
catch ( e )
{ 
	 if ( e == -6 )
	 {
        res3 = true ;
	 }
	 else
	 {
	    println( "Update {$pull_all:{a:3}} failed: " + e ) ;
	 }
}
if ( !res3 )
{
  throw -1	
}
var res4 = false ;
try
{
   varCL.update( {$push_all:{a:2}}) ;
}
catch ( e )
{ 
	 if ( e == -6 )
	 {
        res4 = true ;
	 }
	 else
	 {
	    println( "Update {$push_all:{a:2}} failed: " + e ) ;
	 }
}
if ( !res4 )
{
  throw -1	
}
var res5 = false ;
try
{
   varCL.update( {$pop:{a:[2]}}) ;
}
catch ( e )
{ 
	 if ( e == -6 )
	 {
        res5 = true ;
	 }
	 else
	 {
	    println( "Update {$pop:{a:[2]}} failed: " + e ) ;
	 }
}
if ( !res5 )
{
  throw -1	
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



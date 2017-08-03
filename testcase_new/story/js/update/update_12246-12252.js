// nvi iiii viia // update record.
// normal case.$unset $inc $push $pull $push_all
// $pull_all $pop $addtoset
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
try
{
   varCL.insert({a:2,b:{name:"YoYo",age:23,phone:[12,56,"reqnf"]},c:"jkdi"}) ;
}
catch ( e )
{
   println( "failed to insert record, rc= " + e ) ;
   throw e ;
}

try
{
   varCL.update( {$unset:{c:"jkdi"}}) ;
}
catch ( e )
{
   println( "failed to update record, rc= " + e ) ;
   throw e ;
}

var rc ;
try
{
   rc = varCL.find({c:"jkdi"});
}
catch ( e )
{
   println( "failed to read record, rc= " + e ) ;
   throw e ;
}

var size = 0 ;
while ( true )
{
   var i = rc.next() ;
   if ( !i )
      break ;
   else
      size++ ;
}

if ( 0 != size )
{
   println( "get more than one record: " + size ) ;
   throw -1 ;
}

try
{
   varCL.update( {$inc:{salary:100}}) ;
}
catch ( e )
{
   println( "failed to update record, rc1= " + e ) ;
   throw e ;
}
var rc1 ;
try
{
 rc1 = varCL.find({salary:100});
}
catch ( e )
{
   println( "failed to read record, rc1= " + e ) ;
   throw e ;
}

var size1 = 0 ;
while ( true )
{
   var i = rc1.next() ;
   if ( !i )
      break ;
   else
      size1++ ;
}

if ( 1 != size1 )
{
   println( " get more than one record ,rc1, size: " + size1 ) ;
   throw -1 ;
}

try
{
   varCL.update( {$push:{"b.phone":3}}) ;
}
catch ( e )
{
   println( "failed to update record, rc2= " + e ) ;
   throw e ;
}
var rc2 ;
try
{
 rc2 = varCL.find({"b.phone.3":3});
}
catch ( e )
{
   println( "failed to read record, rc2= " + e ) ;
   throw e ;
}

var size2 = 0 ;
while ( true )
{
   var i = rc2.next() ;
   if ( !i )
      break ;
   else
      size2++ ;
}

if ( 1 != size2 )
{
   println( "get more than one record ,rc2, size: " + size2 ) ;
   throw -1 ;
}
try
{
   varCL.update( {$pull:{"b.phone":3}}) ;
}
catch ( e )
{
   println( "failed to update record, rc3= " + e ) ;
   throw e ;
}
var rc3 ;
try
{
 rc3 = varCL.find({"b.phone.3":3});
}
catch ( e )
{
   println( "failed to read record, rc3= " + e ) ;
   throw e ;
}

var size3 = 0 ;
while ( true )
{
   var i = rc3.next() ;
   if ( !i )
      break ;
   else
      size3++ ;
}

if ( 0 != size3 )
{
   println( "get more than one record ,rc3, size: " + size3 ) ;
   throw -1 ;
}

try
{
   varCL.update( {$push_all:{array:[3,4]}}) ;
}
catch ( e )
{
   println( "failed to update record, rc4= " + e ) ;
   throw e ;
}
var rc4 ;
try
{
 rc4 = varCL.find({array:[3,4]});
}
catch ( e )
{
   println( "failed to read record, rc4= " + e ) ;
   throw e ;
}

var size4 = 0 ;
while ( true )
{
   var i = rc4.next() ;
   if ( !i )
      break ;
   else
      size4++ ;
}

if ( 1 != size4 )
{
   println( " get more than one record ,rc4, size: " + size4 ) ;
   throw -1 ;
}

try
{
   varCL.update( {$pull_all:{array:[3,4]}}) ;
}
catch ( e )
{
   println( "failed to update record, rc5= " + e ) ;
   throw e ;
}
var rc5 ;
try
{
 rc5 = varCL.find({array:[]});
}
catch ( e )
{
   println( "failed to read record, rc5= " + e ) ;
   throw e ;
}

var size5 = 0 ;
while ( true )
{
   var i = rc5.next() ;
   if ( !i )
      break ;
   else
      size5++ ;
}

if ( 1 != size5 )
{
   println( " get more than one record ,rc5, size: " + size5 ) ;
   throw -1 ;
}

try
{
   varCL.update( {$pop:{"b.phone":2}}) ;
}
catch ( e )
{
   println( "failed to update record, rc6= " + e ) ;
   throw e ;
}
var rc6 ;
try
{
 rc6 = varCL.find({"b.phone":[12]});
}
catch ( e )
{
   println( "failed to read record, rc6 = " + e ) ;
   throw e ;
}

var size6 = 0 ;
while ( true )
{
   var i = rc6.next() ;
   if ( !i )
      break ;
   else
      size6++ ;
}

if ( 1 != size6 )
{
   println( " get more than one record ,rc6, size: " + size6 ) ;
   throw -1 ;
}

try
{
   varCL.update( {$addtoset:{"b.phone":[12]}}) ;
}
catch ( e )
{
   println( "failed to update record, rc7= " + e ) ;
   throw e ;
}
var rc7 ;
try
{
 rc7 = varCL.find({"b.phone":[12]});
}
catch ( e )
{
   println( "failed to read record, rc7 = " + e ) ;
   throw e ;
}

var size7 = 0 ;
while ( true )
{
   var i = rc7.next() ;
   if ( !i )
      break ;
   else
      size7++ ;
}

if ( 1 != size7 )
{
   println( " get more than one record ,rc7, size: " + size7 ) ;
   throw -1 ;
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




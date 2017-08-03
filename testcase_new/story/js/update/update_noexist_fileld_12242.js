// update record.
// normal case. $set
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

var insertCount = 1000;
var docs = []
try
{
  for (i = 0; i <insertCount; i++) 
  {
     docs.push({a:i, b:"fdafdsaf$#@$@%$#%#@!$#@!$", c:null, d:{id:1.0,name:"qiu"},e:{ "$binary" : "aGVsbG8gd29ybGQ=", "$type" : "1" }});
  }
  varCL.insert(docs);
}
catch ( e )
{
   println( "failed to insert record, rc= " + e ) ;
   throw e ;
}

try
{
   varCL.update({"$unset":{noexist:""}})
}
catch ( e )
{
   println( "failed to update record, rc= " + e ) ;
   throw e ;
}

var rc ;
try
{
   rc = varCL.find();
}
catch ( e )
{
   println( "failed to read record, rc= " + e ) ;
   throw e ;
}

var recordCount = 0 ;
while ( true )
{
   //var record = eval( "("+ rc.current() +")" );
   var record = rc.current().toObj();
   if ( !compareObj( docs[recordCount], record, false ) )
   {
      println("Record error: the " + recordCount + "th record's a field is not equals " + i);
      throw -1;
   }

   recordCount++;	 
   if ( !rc.next() )
      break ;
}

if ( insertCount != recordCount )
{
   println( "The record count is not equals insert count" ) ;
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

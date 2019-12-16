// list collection.
// normal case.
LISTCLNAME_1 = CHANGEDPREFIX + "bar1";
LISTCLNAME_2 = CHANGEDPREFIX + "bar2";
try
{
   commDropCL( db, COMMCSNAME, LISTCLNAME_1, true, true, "drop cl in the beginning" );
   commDropCL( db, COMMCSNAME, LISTCLNAME_2, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "Failed to clear the collectionspace first :" + e );
   throw e;
}

try
{
   var optionObj = { ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, COMMCSNAME, LISTCLNAME_1, optionObj, true,
      false, "create collecton 1 failed" );
}
catch( e )
{
   println( "failed to create cl, rc= " + e )
   throw e;
}

try
{
   var optionObj = { ReplSize: 0, Compressed: true };
   var varCL1 = commCreateCL( db, COMMCSNAME, LISTCLNAME_2, optionObj, true,
      false, "create collecton 1 failed" );
}
catch( e )
{
   println( "failed to create cl, rc= " + e )
   throw e;
}


try
{
   var cur = db.listCollections();
}
catch( e )
{
   println( "failed to list collections, rc= " + e );
   throw e;
}
var cnt = 0;
while( cur.next() )
{

   var strCSAndCLName = cur.current().toObj()["Name"]
   var strCS = strCSAndCLName.substring( 0, strCSAndCLName.indexOf( "." ) );
   var strCL = strCSAndCLName.substring( strCSAndCLName.indexOf( "." ) + 1 );

   if( strCS == COMMCSNAME && ( strCL == LISTCLNAME_1 || strCL == LISTCLNAME_2 ) )
   {
      cnt++;
   }
}
try
{
   if( 2 != cnt )
   {
      println( "don't have cl: [" + LISTCLNAME_1 + "] and [" + LISTCLNAME_2 + "]" );
      println( "list collection: " + cur );
      throw "inspect collection failed";
   }
}
catch( e )
{
   throw e;
}

try
{
   commDropCL( db, COMMCSNAME, LISTCLNAME_1, false, false,
      "drop cl in the beginning" );
}
catch( e )
{
   println( "failed to drop collections, rc= " + e );
   throw e;
}

try
{
   cur = db.listCollections();
}
catch( e )
{
   println( "failed to list collections, rc= " + e );
   throw e;
}

var cnt = 0;
while( cur.next() )
{


   var strCSAndCLName = cur.current().toObj()["Name"];
   var strCS = strCSAndCLName.substring( 0, strCSAndCLName.indexOf( "." ) );
   var strCL = strCSAndCLName.substring( strCSAndCLName.indexOf( "." ) + 1 );

   if( strCS == COMMCSNAME && strCL == LISTCLNAME_2 )
   {
      ++cnt;
   }
}

try
{
   if( 1 != cnt )
   {
      println( "don't have cl: [" + clName_2 + "]" );
      println( "list collection: " + cur );
      throw "inspect collection failed";
   }
}
catch( e )
{
   throw e;
}

try
{
   commDropCL( db, COMMCSNAME, LISTCLNAME_1, false, true,
      "drop cl in the end" );
   commDropCL( db, COMMCSNAME, LISTCLNAME_2, false, false,
      "drop cl in the end" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

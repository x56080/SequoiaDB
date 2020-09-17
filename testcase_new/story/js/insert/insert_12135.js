main( test );
function test ()
{
   var clName = COMMCLNAME + "_12135";
   // insert record.
   // normal case.
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the beginning" );

   var varCL = commCreateCL( db, COMMCSNAME, clName );
   var veryBigJsonString = "{'_id':1, \"a0\":0";
   var i = 0;
   for( i = 0; i < 1000; i++ )
   {
      veryBigJsonString += ",\"a" + i + "\":" + i;
   }
   veryBigJsonString += "}";
   var veryBigJson = eval( '(' + veryBigJsonString + ')' );

   varCL.insert( veryBigJson );
   if( varCL.find().count() != 1 )
      throw new Error( "varCL.find().count(): " + varCL.find().count() );
   if( !commCompareObject( veryBigJson, varCL.find().next().toObj() ) )
      throw new Error( "veryBigJson: " + JSON.stringify( veryBigJson ) + "\nvarCL.find().next().toObj(): " + JSON.stringify( varCL.find().current().toObj() ) );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop colleciton in the end" );
}

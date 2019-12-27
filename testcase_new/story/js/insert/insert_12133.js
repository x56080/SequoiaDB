/******************************************************************************
@Description : Insert basic: data type
@Modify list :
               2015-01-29 pusheng Ding  Init
******************************************************************************/

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   var clName = COMMCLNAME + "_12133";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in begin" );

   var varCL = commCreateCL( db, COMMCSNAME, clName );

   //int32 && int64
   var records = new Array();

   records[0] = { _id: 1, a: 0, b: "zero" };
   records[1] = { _id: 2, a: -1, b: "plus one" };
   records[2] = { _id: 3, a: 2147483647, b: "max_int32" };
   records[3] = { _id: 4, a: 9223372036854775807, b: "max_int64" };

   for( i = 0; i < records.length; i++ )
   {
      varCL.insert( records[i] );
   }

   var cursor = varCL.find().sort( { _id: 1 } );
   commCompareResults( cursor, records, false );

   //float
   var typeFloat = 123e+50;
   varCL.remove();
   varCL.insert( { a: typeFloat, b: "float" } );
   var sel = varCL.find( { b: "float" } );
   if( sel.next() )
   {
      var ret = sel.current();
      if( ret.toObj()['a'] != typeFloat )
      {
         throw new Error( "ret.toObj()['a']: " + ret.toObj()['a'] + "\ntypeFloat: " + typeFloat );
      }
   }
   sel.close();

   //string
   var typeString = "value:?*";
   varCL.remove();
   varCL.insert( { a: typeString, b: "string" } );
   var sel = varCL.find( { b: "string" } );
   if( sel.next() )
   {
      var ret = sel.current();
      if( ret.toObj()['a'] != typeString )
      {
         throw new Error( "ret.toObj()['a']: " + ret.toObj()['a'] + "\ntypeString: " + typeString );
      }
   }
   sel.close();

   //date && timestamp
   var insDate = "2015-01-29";
   varCL.insert( { a: { "$date": insDate }, b: "date" } );
   var insTimestamp = "2015-01-29-14.30.40.124233";
   varCL.insert( { a: { "$timestamp": insTimestamp }, b: "timestamp" } );
   var sel = varCL.find( { b: "date" } );
   if( sel.next().toObj()['a']['$date'] != insDate )
      throw new Error( "sel.next().toObj()['a']['$date']: " + sel.current().toObj()['a']['$date'] + "\ninsDate: " + insDate );
   var sel = varCL.find( { b: "timestamp" } );
   if( sel.next().toObj()['a']['$timestamp'] != insTimestamp )
      throw new Error( "sel.next().toObj()['a']['$timestamp']: " + sel.current().toObj()['a']['$timestamp'] + "\ninsTimestamp: " + insTimestamp );
   sel.close();

   //OID && binary && regex
   var oidStr = "123abcd00ef12358902300ef";
   var binaryStr = "aGVsbG8gd29ybGQ=";
   var regexStr = "HelloWorld";
   varCL.insert( { a: { "$oid": oidStr }, b: "OID" } );
   varCL.insert( { a: { "$binary": binaryStr, "$type": "1" }, b: "binary" } );
   varCL.insert( { a: { "$regex": regexStr, "$options": "i" }, b: "regex" } );
   var cur = varCL.find( { b: "OID" } );
   if( cur.next().toObj()['a']['$oid'] != oidStr )
      throw new Error( "cur.next().toObj()['a']['$oid']: " + cur.current().toObj()['a']['$oid'] + "\noidStr: " + oidStr );
   var cur = varCL.find( { b: "binary" } );
   if( cur.next().toObj()['a']['$binary'] != binaryStr )
      throw new Error( "cur.next().toObj()['a']['$binary']: " + cur.current().toObj()['a']['$binary'] + "\nbinaryStr: " + binaryStr );
   var cur = varCL.find( { b: "regex" } );
   if( cur.next().toObj()['a']['$regex'] != regexStr )
      throw new Error( "cur.next().toObj()['a']['$regex']: " + cur.current().toObj()['a']['$regex'] + "\nregexStr: " + regexStr );
   cur.close();

   //bool
   varCL.insert( { a: true, b: "booleantrue" } );
   varCL.insert( { a: false, b: "booleanfalse" } );
   var cur = varCL.find( { b: "booleantrue" } );
   if( cur.next().toObj()['a'] != true )
      throw new Error( "cur.next().toObj()['a']: " + cur.current().toObj()['a'] );
   var cur = varCL.find( { b: "booleanfalse" } );
   if( cur.next().toObj()['a'] != false )
      throw new Error( "cur.next().toObj()['a']: " + cur.current().toObj()['a'] );
   cur.close();

   //object
   var insObj = { _id: 1, a: { a: 1, b: "one" }, b: "object" };
   varCL.insert( insObj );
   var cur = varCL.find( { b: "object" } );
   if( !commCompareObject( insObj, cur.next().toObj() ) )
      throw new Error( "insObj: " + JSON.stringify( insObj ) + "\ncur.next().toObj(): " + JSON.stringify( cur.current().toObj() ) );
   cur.close();

   //array
   var ins1 = { _id: 2, a: ["abc", 0, "def"], b: "array1" };
   var ins2 = { _id: 3, a: [{ a1: "array", b1: [1, 2, 3] }, "type array", 123], b: "array2" };
   varCL.insert( ins1 );
   varCL.insert( ins2 );
   var cur = varCL.find( { b: "array1" } );
   if( !commCompareObject( ins1, cur.next().toObj() ) )
      throw new Error( "ins1: " + JSON.stringify( ins1 ) + "\ncur.next().toObj(): " + JSON.stringify( cur.current().toObj() ) );
   var cur = varCL.find( { b: "array2" } );
   if( !commCompareObject( ins2, cur.next().toObj() ) )
      throw new Error( "ins2: " + JSON.stringify( ins2 ) + "\ncur.next().toObj(): " + JSON.stringify( cur.current().toObj() ) );
   cur.close();

   commDropCL( db, COMMCSNAME, clName, false, false, "drop cl in end" );
}

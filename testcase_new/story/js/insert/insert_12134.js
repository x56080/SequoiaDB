main( test );
function test ()
{
   var clName = COMMCLNAME + "_12134";
   // insert record.
   // normal case.
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the beginning" );

   var varCL = commCreateCL( db, COMMCSNAME, clName );
   varCL.insert( [{ a: 1, "_id": { a: 1 }, str: "abcz", integer: 1000, boolean1: true, boolean2: false, nullobj: null, user: { id: 0, name: "name" }, array: [{ name: "qiu", balance: 1.2 }, { name: "shang", balance: -1.2 }] },
   { a: 2, "_id": true, no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: ">女" } },
   { a: 3, "_id": "13000a", "°′″＄￡￥‰％℃¤￠": "○一二三四五六七八九百千万亿兆吉太拍艾分厘毫微零壹贰叁肆伍陆柒捌玖佰仟" },
   { a: 4, _id: 1 },
   { a: 5, _id: { $numberLong: "9223372036854775807" } },
   { a: 6, _id: 12.123 },
   { a: 7, _id: { $decimal: "111111111111111111111111111111111111111111111111111111111111111.111111111111111111111111" } },
   { a: 8, _id: { $date: "2012-01-01" } },
   { a: 9, _id: { $timestamp: "2012-01-01-13.14.26.124233" } },
   { a: 10, _id: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { a: 11, _id: null },
   { a: 12, _id: { $minKey: 1 } },
   { a: 13, _id: { $maxKey: 1 } }] );

   if( varCL.find().count() != 13 )
      throw new Error( "varCL.find().count(): " + varCL.find().count() );
   var lobj1 = { a: 1, "_id": { a: 1 }, str: "abcz", integer: 1000, boolean1: true, boolean2: false, nullobj: null, user: { id: 0, name: "name" }, array: [{ name: "qiu", balance: 1.2 }, { name: "shang", balance: -1.2 }] };
   var lobj2 = { a: 2, "_id": true, no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: ">女" } };
   var lobj3 = { a: 3, "_id": "13000a", "°′″＄￡￥‰％℃¤￠": "○一二三四五六七八九百千万亿兆吉太拍艾分厘毫微零壹贰叁肆伍陆柒捌玖佰仟" };
   var lobj4 = { a: 4, _id: 1 };
   var lobj5 = { a: 5, _id: { $numberLong: "9223372036854775807" } };
   var lobj6 = { a: 6, _id: 12.123 };
   var lobj7 = { a: 7, _id: { $decimal: "111111111111111111111111111111111111111111111111111111111111111.111111111111111111111111" } };
   var lobj8 = { a: 8, _id: { $date: "2012-01-01" } };
   var lobj9 = { a: 9, _id: { $timestamp: "2012-01-01-13.14.26.124233" } };
   var lobj10 = { a: 10, _id: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } };
   var lobj11 = { a: 11, _id: null };
   var lobj12 = { a: 12, _id: { $minKey: 1 } };
   var lobj13 = { a: 13, _id: { $maxKey: 1 } };

   var cur = varCL.find( { "_id": { a: 1 } } );
   if( !commCompareObject( lobj1, cur.next().toObj() ) )
      throw new Error( "lobj1: " + lobj1 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": true } );
   if( !commCompareObject( lobj2, cur.next().toObj() ) )
      throw new Error( "lobj2: " + lobj2 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": "13000a" } );
   if( !commCompareObject( lobj3, cur.next().toObj() ) )
      throw new Error( "lobj3: " + lobj3 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": 1 } );
   if( !commCompareObject( lobj4, cur.next().toObj() ) )
      throw new Error( "lobj4: " + lobj4 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": { $numberLong: "9223372036854775807" } } );
   if( !commCompareObject( lobj5, cur.next().toObj() ) )
      new Error( "lobj5: " + lobj5 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": 12.123 } );
   if( !commCompareObject( lobj6, cur.next().toObj() ) )
      throw new Error( "lobj6: " + lobj6 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": { $decimal: "111111111111111111111111111111111111111111111111111111111111111.111111111111111111111111" } } );
   if( !commCompareObject( lobj7, cur.next().toObj() ) )
      throw new Error( "lobj7: " + lobj7 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": { $date: "2012-01-01" } } );
   if( !commCompareObject( lobj8, cur.next().toObj() ) )
      throw new Error( "lobj8: " + lobj8 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": { $timestamp: "2012-01-01-13.14.26.124233" } } );
   if( !commCompareObject( lobj9, cur.next().toObj() ) )
      throw new Error( "lobj9: " + lobj9 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );
   if( !commCompareObject( lobj10, cur.next().toObj() ) )
      throw new Error( "lobj10: " + lobj10 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": null } );
   if( !commCompareObject( lobj11, cur.next().toObj() ) )
      throw new Error( "lobj11: " + lobj11 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": { $minKey: 1 } } );
   if( !commCompareObject( lobj12, cur.next().toObj() ) )
      throw new Error( "lobj12: " + lobj12 + "\ncur.next().toObj(): " + cur.current().toObj() );

   var cur = varCL.find( { "_id": { $maxKey: 1 } } );
   if( !commCompareObject( lobj13, cur.next().toObj() ) )
      throw new Error( "lobj13: " + lobj13 + "\ncur.next().toObj(): " + cur.current().toObj() );
   cur.close();

   try
   {
      varCL.insert( { _id: { $regex: "^a", $options: "i" } } );
      throw new Error( "need throw error" );
   } catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      varCL.insert( { _id: { $regex: "^a" } } );
      throw new Error( "need throw error" );
   } catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      varCL.insert( { _id: [1, 2] } );
      throw new Error( "need throw error" );
   } catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   commDropCL( db, COMMCSNAME, clName, false, false, "drop cl in the beginning" );
}

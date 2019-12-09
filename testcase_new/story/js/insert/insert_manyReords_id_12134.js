// insert record.
// normal case.
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );
} catch( e )
{
   throw e;
}

try
{
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
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}

try
{
   if( varCL.find().count() != 13 )
      throw "number error";
}
catch( e )
{
   println( "failed to read record, rc= " + e );
   throw e;
}

try
{
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
   if( !compareObj( lobj1, cur.next().toObj() ) )
      throw "lobj1 verify failed";

   var cur = varCL.find( { "_id": true } );
   if( !compareObj( lobj2, cur.next().toObj() ) )
      throw "lobj2 verify failed";

   var cur = varCL.find( { "_id": "13000a" } );
   if( !compareObj( lobj3, cur.next().toObj() ) )
      throw "lobj3 verify failed";

   var cur = varCL.find( { "_id": 1 } );
   if( !compareObj( lobj4, cur.next().toObj() ) )
      throw "lobj4 verify failed";

   var cur = varCL.find( { "_id": { $numberLong: "9223372036854775807" } } );
   if( !compareObj( lobj5, cur.next().toObj() ) )
      throw "lobj5 verify failed";

   var cur = varCL.find( { "_id": 12.123 } );
   if( !compareObj( lobj6, cur.next().toObj() ) )
      throw "lobj6 verify failed";

   var cur = varCL.find( { "_id": { $decimal: "111111111111111111111111111111111111111111111111111111111111111.111111111111111111111111" } } );
   if( !compareObj( lobj7, cur.next().toObj() ) )
      throw "lobj7 verify failed";

   var cur = varCL.find( { "_id": { $date: "2012-01-01" } } );
   if( !compareObj( lobj8, cur.next().toObj() ) )
      throw "lobj8 verify failed";

   var cur = varCL.find( { "_id": { $timestamp: "2012-01-01-13.14.26.124233" } } );
   if( !compareObj( lobj9, cur.next().toObj() ) )
      throw "lobj9 verify failed";

   var cur = varCL.find( { "_id": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );
   if( !compareObj( lobj10, cur.next().toObj() ) )
      throw "lobj10 verify failed";

   var cur = varCL.find( { "_id": null } );
   if( !compareObj( lobj11, cur.next().toObj() ) )
      throw "lobj11 verify failed";

   var cur = varCL.find( { "_id": { $minKey: 1 } } );
   if( !compareObj( lobj12, cur.next().toObj() ) )
      throw "lobj12 verify failed";

   var cur = varCL.find( { "_id": { $maxKey: 1 } } );
   if( !compareObj( lobj13, cur.next().toObj() ) )
      throw "lobj13 verify failed";

   cur.close();
}
catch( e )
{
   throw e;
}

try
{
   varCL.insert( { _id: { $regex: "^a", $options: "i" } } );
   throw "NEED_ERR";
} catch( e )
{
   if( e !== -6 )
   {
      throw e;
   }
}

try
{
   varCL.insert( { _id: { $regex: "^a" } } );
   throw "NEED_ERR";
} catch( e )
{
   if( e !== -6 )
   {
      throw e;
   }
}

try
{
   varCL.insert( { _id: [1, 2] } );
   throw "NEED_ERR";
} catch( e )
{
   if( e !== -6 )
   {
      throw e;
   }
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the beginning" );
}
catch( e )
{
   println( "failed to drop cs, rc= " + e );
   throw e;
}

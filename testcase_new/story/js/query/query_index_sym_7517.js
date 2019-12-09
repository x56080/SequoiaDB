
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
   println( "create cs or cl error" );
   println( "error is " + e );
   throw e;

}

try
{
   for( var i = 1; i <= 100; ++i )
   {
      varCL.insert( { name: "a" + i, number: ( i + 100 ), arr: ["c" + ( i + 200 ), "d" + ( i + 300 )], bson: { bb: "e" + ( i + 400 ), ss: ( i + 500 ) } } );
   }
} catch( e )
{
   println( "insert data error" );
   println( "error is " + e );
   throw e;
}

try
{
   varCL.createIndex( "testIndex", { name: 1 }, false );
} catch( e )
{
   println( "create index error" );
   println( "error is " + e );
   throw e;
}

try
{
   var cur;
   var ix = 1;
   cur = varCL.find().sort( { number: 1 } );
   if( 100 != cur.count() )
      throw "find with the sort option , data's count is error";
   cur = varCL.find().sort( { number: 1 } );
   while( cur.next() )
   {
      if( cur.current().toObj()["name"] != "a" + ix )
         throw "find with the sort option , data is error";
      ix++;
   }


   cur = varCL.find( { $and: [{ number: { $mod: [177, 1] } }] } );
   if( 1 != cur.count() )
      throw "find with options , data's count is error";
   if( cur.current().toObj()["number"] != 178 )
      throw "find with options , data is error";


   cur = varCL.find( { $and: [{ number: { $gt: 150 } }, { number: { $lte: 153 } }, { number: { $ne: 153 } }, { number: { $type: 1, $et: 16 } }, { name: { $exists: 1 } }, { gogo: { $exists: 0 } }] } ).sort( { number: -1 } ).limit( 1 );
   if( cur.size() != 1 )
      throw "find with many options , but result's number have error";
   cur = varCL.find( { $and: [{ number: { $gt: 150 } }, { number: { $lte: 153 } }, { number: { $ne: 153 } }, { number: { $type: 1, $et: 16 } }, { name: { $exists: 1 } }, { gogo: { $exists: 0 } }] } ).sort( { number: -1 } ).limit( 1 );
   if( cur.current().toObj()["name"] != "a52" )
      throw "find with many options , but result have error";



   cur = varCL.find( { $or: [{ $not: [{ arr: { $size: 1, $et: 2 } }] }, { $and: [{ name: { $regex: '7$', $options: 'i' } }, { number: { $gte: 197 } }] }] } );
   if( cur.current().toObj()["name"] != "a97" )
      throw "find with $or have error";
   if( cur.count() != 1 )
      throw "find with $or , the result number have error";

   //,{number:{$et:197}}
   cur = varCL.find( { $and: [{ bson: { $elemMatch: { bb: { $regex: '^e.*7$', $options: 's' } } } }, { number: { $et: 197 } }] } );
   if( cur.count() != 1 )
      throw "find with $elemMatch , result number have error";
   cur = varCL.find( { $and: [{ bson: { $elemMatch: { bb: { $regex: '^e.*7$', $options: 's' } } } }, { number: { $et: 197 } }] } );
   if( cur.current().toObj()["name"] != "a97" )
      throw "find with $elemMatch , have error";
} catch( e )
{
   println( "find have error" );
   println( "error is " + e );
   throw e;
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );
}
catch( e )
{
   if( e != -34 )
   {
      println( "unexpected err happened when clear cs:" + e );
      throw e;
   }
}

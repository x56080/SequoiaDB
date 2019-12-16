
// clear
commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

// create cs, cl
var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, false, "create cs and cl in begin" );

try
{
   for( var i = 0; i < 1000; i++ )
   {
      if( i < 500 )
      {
         varCL.insert( { no: i } );
      }
      else
      {
         varCL.insert( { no: i, name: "foo" + i } );
      }
   }
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}
try
{
   varCL.createIndex( "testIndex", { name: -1 }, false );
}
catch( e )
{
   println( "failed to create index, rc= " + e );
   throw e;
}
try
{
   rc = varCL.find( { $or: [{ no: { $gt: 250, $lt: 500 } }, { no: { $gt: 500, $lt: 900 } }], name: { $regex: "foo*" }, no: { $mod: [5, 3] }, $and: [{ no: { $gt: 800 } }, { no: { $lt: 850 } }], name: { $nin: ["foo898", "foo893", "foo888"] } } );
}
catch( e )
{
   println( "failed to find records after create index, rc= " + e );
   throw e;
}

var count = 0;
try
{
   count = rc.count();
   println( "count1: " + count );
}
catch( e )
{
   println( "get rc count after create index failed: " + e );
   throw e;
}

if( count != 10 )
{
   println( "return wrong number of records after create index" );
   println( varCL.find( { $or: [{ no: { $gt: 250, $lt: 500 } }, { no: { $gt: 500, $lt: 900 } }], name: { $regex: "foo*" }, no: { $mod: [5, 3] }, $and: [{ no: { $gt: 800 } }, { no: { $lt: 850 } }], name: { $nin: ["foo898", "foo893", "foo888"] } } ) );
   throw -1;
}

try
{
   varCL.dropIndex( "testIndex" );
}
catch( e )
{
   println( "failed to drop index, rc= " + e );
   throw e;
}

var looptime = 0;
while( looptime < 1 )
{
   ++looptime;
   try
   {
      rc = varCL.find( { $or: [{ no: { $gt: 250, $lt: 500 } }, { no: { $gt: 500, $lt: 900 } }], name: { $regex: "foo*" }, no: { $mod: [5, 3] }, $and: [{ no: { $gt: 800 } }, { no: { $lt: 850 } }], name: { $nin: ["foo898", "foo893", "foo888"] } } );
   }
   catch( e )
   {
      println( "failed to find record after drop index, rc= " + e );
      throw e;
   }

   try
   {
      count = rc.count();
      println( "count2: " + count );
   }
   catch( e )
   {
      if( e == -48 ) // query from secondary node when it's index is deleted after query
      {
         println( " get rc count after drop index failed: " + e + ", ignored" );
         looptime = 0;
      }
      else
      {
         println( "get rc count failed after drop index: " + e );
         throw e;
      }
   }
}

if( count != 10 )
{
   println( "return wrong number of records after drop index" );
   println( varCL.find( { $or: [{ no: { $gt: 250, $lt: 500 } }, { no: { $gt: 500, $lt: 900 } }], name: { $regex: "foo*" }, no: { $mod: [5, 3] }, $and: [{ no: { $gt: 800 } }, { no: { $lt: 850 } }], name: { $nin: ["foo898", "foo893", "foo888"] } } ) );
   throw -1;
}

// clear
commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );

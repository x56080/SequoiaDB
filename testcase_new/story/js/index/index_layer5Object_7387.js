/***************************************************************************
@Description : The Object have 5 layer nest, and the create index .
               such:[db.CS.CL.createIndex("arrLay5Index",
                    {"object1.object2.object3.object4.1":1}, true, true )]
@Modify list :
               2014-5-21  xiaojun Hu  Init
****************************************************************************/
function main ( db )
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false, "create collection" );

   // insert data to SDB
   try
   {
      idxCL.insert( { no: 001, name: "A", age: 2, "object1": { "object2": { "object3": { "object4": { "object5": "5LayerObject" } } } } } );
      var i = 0;
      do
      {
         var count = idxCL.count( { no: 001, name: "A", age: 2, "object1": { "object2": { "object3": { "object4": { "object5": "5LayerObject" } } } } } );
         ++i;
      } while( i < 10 )
      if( 1 != count )
      {
         println( "Wrong number of record :" + count );
         throw "ErrNumRecord"
      }
   }
   catch( e )
   {
      println( "Failed to insert date after create index : " + e );
      throw e;
   }

   // create index
   createIndex( idxCL, "ObjLay5Index", { "object1.object2.object3.object4.object5": 1 }, true, true );

   // inspect index
   try
   {
      inspecIndex( idxCL, "ObjLay5Index", "object1.object2.object3.object4.object5", 1, true, true );
   }
   catch( e )
   {
      if( "ErrIdxName" != e )
      {
         throw e;
      }
   }

   // drop collection in clean
   commDropCL( db, csName, clName, false, false,
      "drop colleciton in the end" );
}

try
{
   main( db );
   db.close();
}
catch( e )
{
   throw e;
}

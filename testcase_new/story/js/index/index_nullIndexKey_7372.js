/******************************************************************************
@Description : create index when the index field object is null.
               such: [db.CS.CL.createIndex{"testIndex",{},false,false}]
@Modify list :
               2014-5-20  xiaojun Hu  Modify
******************************************************************************/

function main ( db )
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false, "create collection" );

   // insert data to SDB
   idxCL.insert( { a: 1 } );

   // create index
   createIndex( idxCL, "testindex", {}, false, false, -6 );

   // inspect the index
   try
   {
      inspecIndex( idxCL, "testindex", "a", 1, false );
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

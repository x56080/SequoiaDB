/*******************************************************************************
@Description : When creating index , we specify the sort key :1/-1. If not equal
               1 and -1, there is a error.
@Modify list :
               2014-5-19  xiaojun Hu  Create
*******************************************************************************/
function main ( db )
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, -1, true, true, false, "create collection" );

   //insert data after create index
   try
   {
      idxCL.insert( { no: 001, name: "A", Des: "a" } );
      idxCL.insert( { no: 002, name: "B", Des: "b" } );
      idxCL.insert( { no: 003, name: "C", Des: "c" } );
      idxCL.insert( { no: 004, name: "D", Des: "d" } );
      idxCL.insert( { no: 005, name: "E", Des: "e" } );
      var count = idxCL.count();
      var i = 0;
      do
      {
         cnt = count;
         ++i;
      } while( i < 10 )
      if( 5 != count )
      {
         println( "Wrong number of record :" + count );
         throw "ErrNumRecord";
      }
   }
   catch( e )
   {
      println( "Failed to insert date after create index : " + e );
      throw e;
   }

   // create index
   try
   {
      createIndex( idxCL, "noIndex", { no: 1 }, false, false );
      // create index. specify the sort key:-1
      createIndex( idxCL, "nameIndex", { name: -1 }, false, false );
      // create index. specify the sort key:5
      createIndex( idxCL, "desIndex", { Des: 5 }, false, false );
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "Failed to create index, rc = " + e );
         throw e;
      }
   }

   // create index. specify the sort key:-5
   try
   {
      idxCL.createIndex( "desIndex", { Des: -5 } );
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "Error, specify the sort key:-5, rc=" + e );
         throw e;
      }
   }


   // create index. specify the sort key:0
   try
   {
      idxCL.createIndex( "desIndex", { Des: 0 } );
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "Erro, specify the sort key:0, rc=" + e );
         throw e;
      }
   }


   // inspect the index
   try
   {
      inspecIndex( idxCL, "noIndex", "no", 1, false, false );
      inspecIndex( idxCL, "nameIndex", "name", -1, false, false );
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

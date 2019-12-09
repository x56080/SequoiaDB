/* *****************************************************************************
@discretion: index in cl,create index and delete index after rename cl,
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      var clName = CHANGEDPREFIX + "_renamecl16057";
      var newCLName = CHANGEDPREFIX + "_newcl16057";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
      var dbcl = commCreateCL( db, COMMCSNAME, clName, 0 );

      println( "---begin to insert data and create index" );
      var recordNums = 2000;
      var indexName1 = "testindex1";
      var indexName2 = "testindex2";
      var indexName3 = "testindex3";
      insertData( dbcl, recordNums );
      dbcl.createIndex( indexName1, { no: 1 } );
      dbcl.createIndex( indexName2, { a: 1 } );

      println( "---begin to rename cl" );
      db.getCS( COMMCSNAME ).renameCL( clName, newCLName );

      println( "---begin to create and drop indexs" );
      var newcl = db.getCS( COMMCSNAME ).getCL( newCLName );
      newcl.dropIndex( indexName1 );
      newcl.createIndex( indexName3, { user: 1 }, true );

      println( "---begin to check result" );
      checkRenameCLResult( COMMCSNAME, clName, newCLName );
      checkIndexResult( newcl, indexName3, indexName1 );

      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );
   }
   catch( e )
   {
      throw e;
   }
}

function checkIndexResult ( newcl, expIndexName, expDeleteIndexName )
{
   try
   {
      //check index
      var indexName = newcl.find( { user: "test1" } ).explain().current().toObj().IndexName;
      if( indexName !== expIndexName )
      {
         throw buildException( "check find by index", null, "check find by index",
            indexName1, expIndexName );
      }

      try
      {
         newcl.getIndex( expDeleteIndexName );
         throw "need throw error";
      }
      catch( e )
      {
         if( e !== -47 )
         {
            throw buildException( "check Index:", e );
         }
      }



   }
   catch( e )
   {
      throw buildException( "checkIndexResult", e )
   }
}

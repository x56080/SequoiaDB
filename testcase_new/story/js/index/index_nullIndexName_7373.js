/********************************************************************************
@Description : abnormal test : db.CS.CL.createIndex("",{a:1},false)
@Modify list :
               2014-5-20  xiaojun Hu  Modify
********************************************************************************/
main();
function main ()
{
   var clName = "cl7373";
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );
   var index = "";
   for( var i = 0; i < 1024; i++ )
   {
      index += 't';
   }

   // create collection
   var idxCL = commCreateCL( db, csName, clName, -1, true, true, false, "create collection" );

   // insert data to SDB
   idxCL.insert( { a: 1 } );

   // create index
   createIndex( idxCL, "" );
   createIndex( idxCL, index );

   // drop collection in clean
   commDropCL( db, csName, clName, false, false, "drop collection in the end" );
}

function createIndex ( cl, idxName )
{
   try
   {
      cl.createIndex( idxName, { a: 1 } );
      throw "expected failure but found succeed. index name = " + idxName;
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw buildException( "createIndex", e, "create index has failed", -6, e );
      }
   }
}


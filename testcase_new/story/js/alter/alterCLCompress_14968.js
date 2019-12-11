/* *****************************************************************************
@discretion: cl enable compression, then insert and query data
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclcompression_14968";

try
{
   main( db );
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ( db )
{
   try
   {
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }
      //clean environment before test
      commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

      //create cl
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { Compressed: false } );

      //enable compression
      dbcl.enableCompression();
      checkAlterResult( clName, "AttributeDesc", "Compressed" );
      checkAlterResult( clName, "CompressionTypeDesc", "lzw" );

      //insert data and query data
      insertAndQueryRecs( dbcl );

      //clean
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( db != null )
      {
         db.close()
      }
   }
}

function insertAndQueryRecs ( cl )
{
   println( "\n---begin to insert " );
   var expRecs = [];
   for( var i = 0; i < 10000; i++ )
   {
      var rec = { a: i, b: i, c: i };
      expRecs.push( rec );
   }
   cl.insert( expRecs );

   println( "\n---begin to query and check datas. " );
   var rc = cl.find().sort( { a: 1 } );
   checkRec( rc, expRecs );
}

function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   if( actRecs.length !== expRecs.length )
   {
      println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
      throw buildException( "check count", null, "",
         expRecs.length, actRecs.length );
   }

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
            throw buildException( "checkRec()", "rec ERROR" );
         }
      }
   }
}

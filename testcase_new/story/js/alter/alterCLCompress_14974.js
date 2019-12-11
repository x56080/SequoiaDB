/* *****************************************************************************
@discretion: disable compression, then alter compressionType
@author��2018-4-26 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclcompression_14974";

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
      var dbcl = commCreateCL( db, COMMCSNAME, clName );

      //enable compression
      var compressionType = "lzw";
      dbcl.setAttributes( { CompressionType: compressionType } );
      checkAlterResult( clName, "AttributeDesc", "Compressed" );
      checkAlterResult( clName, "CompressionTypeDesc", compressionType );

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


/******************************************************************************
@Description :   seqDB-15993: 创建集合时，创建16个自增字段 
@Modify list :   2018-10-17    xiaoni Zhao  Init
******************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_15993";

   commDropCL( db, COMMCSNAME, clName );

   var autoIncrements = getAutoIncrements();

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: autoIncrements } );

   //check autoIncrement and sequence
   var sequenceNames = getSequenceNames( COMMCSNAME, clName, autoIncrements );
   var expIncrements = getExpIncrements( sequenceNames, autoIncrements );
   checkAutoIncrementonCL( COMMCSNAME, clName, expIncrements );

   for( var i in sequenceNames )
   {
      checkSequence( sequenceNames[i], {} );
   }

   //insert records
   var rc = dbcl.find().sort( { "id1": 1 } );
   var expRecs = new Array();
   for( var i = 0; i < 100; i++ )
   {
      dbcl.insert( { "a": i } );
      expRecs.push( {
         "a": i, "id0": 1 + i, "id1": 1 + i, "id2": 1 + i, "id3": 1 + i, "id4": 1 + i,
         "id5": 1 + i, "id6": 1 + i, "id7": 1 + i, "id8": 1 + i, "id9": 1 + i,
         "id10": 1 + i, "id11": 1 + i, "id12": 1 + i, "id13": 1 + i,
         "id14": 1 + i, "id15": 1 + i
      } );
   }
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

function getAutoIncrements ()
{
   try
   {
      var autoIncrements = new Array();
      for( var i = 0; i < 16; i++ )
      {
         autoIncrements.push( { Field: "id" + i } );
      }
      return autoIncrements;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

function getSequenceNames ( csName, clName, autoIncrements )
{
   try
   {
      var sequenceNames = new Array();
      var clID = getCLID( csName, clName );
      for( var i in autoIncrements )
      {
         sequenceNames.push( "SYS_" + clID + "_" + autoIncrements[i].Field + "_SEQ" );
      }
      return sequenceNames;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

function getExpIncrements ( sequenceNames, autoIncrements )
{
   try
   {
      var expIncrements = new Array();
      for( var i in autoIncrements )
      {
         expIncrements.push( { Field: autoIncrements[i].Field, SequenceName: sequenceNames[i] } );
      }
      return expIncrements;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

/******************************************************************************
@Description: Procedure common functions
@modify list:
   2014-3-14 Jianhui Xu  Init
******************************************************************************/

/******************************************************************************
@Description: clean procedure
@author: Jianhui Xu
@parameter:
   filter : procedure name filter
******************************************************************************/
function fmpCleanProcedures ( db, filter )
{
   if( filter == undefined ) { filter = ""; }

   var procedures = commGetProcedures( db, filter );
   for( var i = 0; i < procedures.length; ++i )
   {
      try
      {
         db.removeProcedure( procedures[i] );
      }
      catch( e )
      {
         println( "fmpCleanProcedures remove procedure " + procedures[i] + " failed: " + e );
         if( e != -233 )
         {
            throw e;
         }
      }
   }
}

/******************************************************************************
@Description: clean procedure
@author: Jianhui Xu
@parameter:
   nameArray : name array
   ignoreNotExist : true/false, default is false
******************************************************************************/
function fmpRemoveProcedures ( nameArray, ignoreNotExist )
{
   if( ignoreNotExist == undefined ) { ignoreNotExist = false; }

   for( var i = 0; i < nameArray.length; ++i )
   {
      try
      {
         db.removeProcedure( nameArray[i] );
      }
      catch( e )
      {
         if( !ignoreNotExist || e != -233 )
         {
            println( "Failed to remove function[" + nameArray[i] + "],e=" + e );
            throw e;
         }
      }
   }
}

//add by TingYU
function checkResult ( rc, expRsts )
{
   //get actual records to array
   var actRsts = [];
   while( rc.next() )
   {
      actRsts.push( rc.current().toObj() );
   }

   //check count
   if( actRsts.length !== expRsts.length )
   {
      println( "\nactual procedures= " + JSON.stringify( actRsts ) + "\n\nexpect procedures= " + JSON.stringify( expRsts ) );
      throw buildException( "check procedures number", null, "",
         expRsts.length, actRsts.length );
   }

   //check every records every fields
   for( var i in expRsts )
   {
      var actRec = actRsts[i];
      var expRec = expRsts[i];
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th procedures, in field '" + f + "'" );
            println( "\nactual procedure= " + JSON.stringify( actRsts ) + "\n\nexpect procedures= " + JSON.stringify( expRsts ) );
            throw buildException( "checkResult()", "procedure ERROR" );
         }
      }
   }
}

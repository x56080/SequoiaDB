import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

function getRandomInt ( min, max ) // [min, max)
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}

function RecycleChecker( db, csName, clName, groupName )
{
   this.csName = csName;
   this.clName = clName;
   this.groupName = groupName;
   this.db = db;

   this.checkTotalDeletingRecords = function( expectRecords, timeoutSec )
   {
      var success = false;
      var costSec = 0;
      var result = -1;
      do
      {
         var cl = this.db.getCS( this.csName ).getCL( this.clName );
         var cursor = cl.getDetail();
         while (cursor.next())
         {
            var detail = cursor.current().toObj().Details[0];
            if (detail.GroupName == groupName)
            {
               result = detail.TotalDeletingRecords;
               break;
            }
         }
         cursor.close();

         if (result == expectRecords)
         {
            success = true;
            break;
         }
         else
         {
            ++costSec;
            sleep( 1000 );
         }
      }
      while (costSec < timeoutSec);
      println( "check cost: " + costSec + "s" );
      if (!success)
      {
         assert.equal( result, expectRecords );
      }
   }

   this.checkTotalOverflowRecords = function( expectRecords )
   {
      var result = -1;
      var cl = this.db.getCS( this.csName ).getCL( this.clName );
      var cursor = cl.getDetail();
      while (cursor.next())
      {
         var detail = cursor.current().toObj().Details[0];
         if (detail.GroupName == groupName)
         {
            result = detail.TotalOverflowRecords;
            break;
         }
      }
      cursor.close();

      assert.equal( result, expectRecords );
   }

   this.cleanUp = function()
   {
   }
}

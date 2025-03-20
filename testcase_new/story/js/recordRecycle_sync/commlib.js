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
   // init cmd object
   var node = db.getRG( groupName ).getMaster();
   var remote = new Remote( node.getHostName() );
   this.cmd = remote.getCmd();
   // init dump command
   var dumpCommand = '';
   var installDir = cmd.run("grep 'INSTALL_DIR' /etc/default/sequoiadb | " +
                            "awk -F '=' '{ print $2 }'");
   installDir = installDir.slice(0, -1)
   dumpCommand += installDir + "/bin/sdbdmsdump ";
   var nodeDir = node.getDetailObj().toObj().dbpath;
   dumpCommand += "-d " + nodeDir + " ";
   this.outputFile = "/tmp/" + csName + "." + clName + ".dump";
   dumpCommand += "-o " + this.outputFile + " ";
   dumpCommand += "-c " + csName + " ";
   dumpCommand += "-l " + clName + " ";
   dumpCommand += "-t true -p true -a dump";
   this.dumpCommand = dumpCommand;

   println( dumpCommand );

   this.checkTotalDeletingRecords = function( expectRecords, timeoutSec )
   {
      var success = false;
      var costSec = 0;
      do
      {
         this.cmd.run( this.dumpCommand );
         var parseCommand = "grep -i 'Total deleting record' " +
                            this.outputFile + ".0 | awk '{print $4}'";
         var result = this.cmd.run( parseCommand );
         result = result.slice(0, -1);

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

   this.cleanUp = function()
   {
      this.cmd.run( "rm " + this.outputFile + ".0" );
   }
}

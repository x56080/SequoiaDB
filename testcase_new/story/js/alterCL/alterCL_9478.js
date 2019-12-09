/******************************************************************************
@Description : jira1205 seqDB-9478 domain.alter不检查group上是否存在数据
@author :Shitong Wen
               
******************************************************************************/

function main ()
{
   if( commIsStandalone( db ) )
      //throw "NoEnoughGroup";
      return;
   else
   {
      println( "---begin to get group" );
      var group = new Array();
      group = getGroup( db );
      if( group.length <= 1 )
         //throw "NoEnoughGroup";
         return;
      else
      {
         var newCS = CHANGEDPREFIX + '_newcs';
         var newCL = CHANGEDPREFIX + '_newcl';
         var domainName = CHANGEDPREFIX + '_domain';

         // Drop CS and domain in the beginning
         println( "---begin to drop cs and domain in the beginning" );
         commDropDomain( db, domainName );

         //create  domain cs cl 
         println( "---begin to create  a domain with contain two group " );
         commCreateDomain( db, domainName, [group[0], group[1]] );
         commCreateCS( db, newCS, false, "create CS specify domain",
            { "Domain": domainName } );
         db.getCS( newCS ).createCL( newCL, { Group: group[0] } );

         try
         {

            //alter ome group  
            println( "---begin to alter one group --> " + group[1] );
            db.getDomain( domainName ).alter( { Groups: [group[1]] } );
            throw "not throw error";

         } catch( e )
         {
            if( e != -256 )
            {
               throw buildException( "check return code", "",
                  "testDomain.alter({Groups:[group[1]]}",
                  "throw -256", e );
            }

         }

         //split data
         println( "---begin to split data " );
         for( var i = 0; i < 100; i++ ) { db.getCS( newCS ).getCL( newCL ).insert( { key: i } ) };
         db.getCS( newCS ).getCL( newCL ).alter( { ShardingKey: { key: 1 }, ShardingType: "hash" } );
         db.getCS( newCS ).getCL( newCL ).split( group[0], group[1], 50 );

         try
         {
            //remove ome group    
            println( "---begin to alter the other one group " + group[0] );
            db.getDomain( domainName ).alter( { Groups: [group[0]] } );
            throw "not throw error";
         } catch( e )
         {
            if( e != -256 )
            {
               throw buildException( "check return code", "",
                  'testDomain.alter({Groups:[group[0]}})',
                  "throw -256", e );

            }
         }


         //clean environment
         commDropDomain( db, domainName );

      }

   }

}
main();
db.close();

// Get group from Sdb
function getGroup ( db )
{
   try
   {
      var listGroups = db.listReplicaGroups();
      var groupArray = new Array();
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()['GroupID'] >= DATA_GROUP_ID_BEGIN )
         {
            groupArray.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      return groupArray;
   }
   catch( e )
   {
      println( "Failed to get groups from sdb, rc = " + e );
      throw e;
   }
}
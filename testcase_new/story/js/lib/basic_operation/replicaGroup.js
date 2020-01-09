/* *****************************************************************************
@discription: return object of ReplicaGroup
@author: yinzhen
@parameter:
   group: primitive SdbReplicaGroup object
***************************************************************************** */
function ReplicaGroup ( group )
{
   this.createNode =
      function( host, service, dbpath, config )
      {
         if( config === undefined ) { config = {}; }
         try
         {
            group.createNode( host, service, dbpath, config );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.getNode =
      function( hostname, servicename )
      {
         try
         {
            var node = group.getNode( hostname, servicename );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Node( node );
      }

   this.removeNode =
      function( host, service, options )
      {
         if( config === undefined ) { options = {}; }
         try
         {
            group.removeNode( host, service, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.getMaster =
      function()
      {
         try
         {
            var node = group.getMaster();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Node( node );
      }

   this.getDetail =
      function()
      {
         try
         {
            var obj = group.getDetail();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.getSlave =
      function( positions )
      {
         try
         {
            if( positions === undefined )
            {
               var node = group.getSlave();
            }
            else
            {
               var node = group.getSlave( positions );
            }
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Node( node );
      }

   this.reelect =
      function( options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            group.reelect( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.start =
      function()
      {
         try
         {
            group.start();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.stop =
      function()
      {
         try
         {
            group.stop();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

}

/* *****************************************************************************
@discription: return object of ReplicaGroup
@author: yinzhen
@parameter:
   group: primitive SdbReplicaGroup object
***************************************************************************** */
function ReplicaGroup ( group )
{
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
}

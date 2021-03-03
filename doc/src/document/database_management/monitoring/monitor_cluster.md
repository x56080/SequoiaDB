1.  连接到协调节点

    ```lang-javascript
    $ /opt/sequoiadb/bin/sdb
    > var db = new Sdb( "localhost", 11810 )
    ```

2.  得到集群状态

    ```lang-javascript
    > db.listReplicaGroups()
    {
      "Group": [
        {
          "dbpath": "/opt/sequoiadb/database/cata/11800",
          "HostName": "hostname1",
          "Service": [
            ...
          ],
          "NodeID": 1
        },
        {
          "HostName": "hostname2",
          "dbpath": "/opt/sequoiadb/database/cata/11800",
          "Service": [
            ...
          ],
          "NodeID": 2
        },
        {
          "HostName": "hostname3",
          "dbpath": "/opt/sequoiadb/database/cata/11800",
          "Service": [
            ...
          ],
          "NodeID": 3
        }
      ],
      "GroupID": 1,
      "GroupName": "SYSCatalogGroup",
      "PrimaryNode": 1,
      "Role": 2,
      "Status": 1,
      "Version": 3,
      "_id": {
        "$oid": "558b9264de349a1b87451a1d"
      }
    }
    {
      "Group": [
        {
          "HostName": "hostname1",
          "dbpath": "/opt/sequoiadb/database/data/21100",
          "Service": [
            ...
          ],
          "NodeID": 1000
        },
        {
          "HostName": "hostname2",
          "dbpath": "/opt/sequoiadb/database/data/21100",
          "Service": [
            ...
          ],
          "NodeID": 1001
        },
        {
          "HostName": "hostname3",
          "dbpath": "/opt/sequoiadb/database/data/21100",
          "Service": [
            ...
          ],
          "NodeID": 1002
        }
      ],
      "GroupID": 1000,
      "GroupName": "group1",
      "PrimaryNode": 1001,
      "Role": 0,
      "Status": 1,
      "Version": 4,
      "_id": {
        "$oid": "558b9295de349a1b87451a21"
      }
    }
    ...
    ```

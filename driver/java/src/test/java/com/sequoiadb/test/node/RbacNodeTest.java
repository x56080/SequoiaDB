/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = RbacNodeTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.node;

import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.TestConfig;
import com.sequoiadb.test.rbac.Privilege;
import com.sequoiadb.test.rbac.Role;
import org.bson.BasicBSONObject;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.Collections;

import static com.sequoiadb.test.AssertUtil.assertNotThrows;
import static com.sequoiadb.test.AssertUtil.assertSDBError;
import static org.junit.Assert.assertEquals;

public class RbacNodeTest {
    private Sequoiadb adminDb;
    private Sequoiadb userDb;

    private final String roleName = "test_role";

    private final Role role1 = Role.builder()
            .role(roleName)
            .addPrivilege(
                    Privilege.builder()
                            .resource(true)
                            .addActions("createRG", "createNode", "alterNode", "alterRG")
                            .build()
            )
            .build();

    private final Role role2 = Role.builder()
            .role(roleName)
            .addPrivilege(
                    Privilege.builder()
                            .resource(true)
                            .addActions("createRG", "createNode", "list", "snapshot")
                            .build()
            )
            .build();

    private final String username = "test_user";

    private final String password = "test";

    @Before
    public void setUp() {
        adminDb = new Sequoiadb(
                TestConfig.getRbacCoordHost(),
                TestConfig.getRbacCoordPort(),
                TestConfig.getRbacRootUsername(),
                TestConfig.getRbacRootPassword()
        );

        adminDb.createRole(role1.toBson());

        BasicBSONObject options = new BasicBSONObject("Roles", Collections.singleton(role1.getRole()));
        adminDb.createUser(username, password, options);

        userDb = new Sequoiadb(
                TestConfig.getRbacCoordHost(),
                TestConfig.getRbacCoordPort(),
                username,
                password
        );
    }

    @After
    public void tearDown() {
        adminDb.removeUser(username, password);
        adminDb.dropRole(role1.getRole());
        userDb.close();
        adminDb.close();
    }

    @Test
    public void createNodeTest() {
        String newGroupName = "group_test";
        try {
            ReplicaGroup replicaGroup = userDb.createReplicaGroup(newGroupName);
            Node node = replicaGroup.createNode(
                    TestConfig.getRbacCoordHost(),
                    12920,
                    TestConfig.getRbacNewNodeDbPathPrefix() + "/12920"
            );

            assertNotThrows(node::getNodeId);
            assertSDBError(SDBError.SDB_NO_PRIVILEGES, node::getStatus);

            adminDb.updateRole(roleName, role2.toBson());
            assertEquals(Node.NodeStatus.SDB_NODE_ACTIVE, node.getStatus());
        } finally {
            adminDb.removeReplicaGroup(newGroupName);
        }
    }

    @Test
    public void setLocationTest() {
        String newGroupName = "group_test";
        try {
            ReplicaGroup replicaGroup = userDb.createReplicaGroup(newGroupName);
            Node node = replicaGroup.createNode(
                    TestConfig.getRbacCoordHost(),
                    12920,
                    TestConfig.getRbacNewNodeDbPathPrefix() + "/12920"
            );
            assertNotThrows(() -> node.setLocation("shanghai"));
            assertNotThrows(() -> replicaGroup.setActiveLocation("shanghai"));
        } finally {
            adminDb.removeReplicaGroup(newGroupName);
        }
    }

}

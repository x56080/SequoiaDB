package com.sequoiadb.test.rg;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.TestConfig;
import com.sequoiadb.test.rbac.Privilege;
import com.sequoiadb.test.rbac.Role;
import org.bson.BasicBSONObject;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.Collections;

import static org.junit.Assert.*;

public class RbacRgTest {

    private Sequoiadb adminDb;
    private Sequoiadb userDb;

    private final String roleName = "test_role";

    private final Role role1 = Role.builder()
            .role(roleName)
            .addPrivilege(
                    Privilege.builder()
                            .resource(true)
                            .addActions("createRG")
                            .build()
            )
            .build();

    private final Role role3 = Role.builder()
            .role(roleName)
            .addPrivilege(
                    Privilege.builder()
                            .resource(true)
                            .addActions("createRG", "list", "createNode", "removeNode")
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
    public void onlyNeedCreateRGPrivilegesTest() {
        String newGroupName = "group_test";
        try {
            ReplicaGroup replicaGroup = userDb.createReplicaGroup(newGroupName);

            assertEquals(newGroupName, replicaGroup.getGroupName());
            assertFalse(replicaGroup.isCatalog());
        } finally {
            adminDb.removeReplicaGroup(newGroupName);
        }
    }

    @Test
    public void needListPrivilegesTest() {
        String newGroupName = "group_test";
        try {
            ReplicaGroup replicaGroup = userDb.createReplicaGroup(newGroupName);

            assertNotPrivileges(replicaGroup::getId);
            assertNotPrivileges(replicaGroup::getDetail);
            assertNotPrivileges(replicaGroup::getMaster);
            assertNotPrivileges(replicaGroup::getSlave);
            assertNotPrivileges(() -> replicaGroup.getNode(TestConfig.getRbacCoordHost(), 12910));

            adminDb.updateRole(roleName, role3.toBson());
            replicaGroup.createNode(
                    TestConfig.getRbacCoordHost(),
                    12930,
                    TestConfig.getRbacNewNodeDbPathPrefix() + "/12930"
            );

            replicaGroup.getId();
            replicaGroup.getDetail();
            assertNotSDBError(SDBError.SDB_NO_PRIVILEGES, replicaGroup::getMaster);
            replicaGroup.getSlave();
            replicaGroup.getNode(TestConfig.getRbacCoordHost(), 12930);
        } finally {
            adminDb.removeReplicaGroup(newGroupName);
        }
    }

    @Test
    public void needNodePrivilegesTest() {
        String newGroupName = "group_test";
        try {
            ReplicaGroup replicaGroup = userDb.createReplicaGroup(newGroupName);

            assertNotPrivileges(() -> replicaGroup.createNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    TestConfig.getRbacNewNodeDbPathPrefix() + "/13920"
            ));
            assertNotPrivileges(() -> replicaGroup.detachNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    new BasicBSONObject()
            ));
            assertNotPrivileges(() -> replicaGroup.attachNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    new BasicBSONObject()
            ));

            adminDb.updateRole(roleName, role3.toBson());
            replicaGroup.createNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    TestConfig.getRbacNewNodeDbPathPrefix() + "/13920"
            );
            assertNotSDBError(SDBError.SDB_NO_PRIVILEGES, () -> replicaGroup.detachNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    new BasicBSONObject("KeepData", false)
            ));
            assertNotSDBError(SDBError.SDB_NO_PRIVILEGES, () -> replicaGroup.attachNode(
                    TestConfig.getRbacCoordHost(),
                    13920,
                    new BasicBSONObject()
            ));
        } finally {
            adminDb.removeReplicaGroup(newGroupName);
        }
    }

    private void assertNotPrivileges(Runnable action) {
        assertSDBError(SDBError.SDB_NO_PRIVILEGES, action);
    }

    private void assertSDBError(SDBError expected, Runnable action) {
        try {
            action.run();
            fail();
        } catch (BaseException e) {
            assertEquals(expected.getErrorCode(), e.getErrorCode());
        }
    }

    private void assertNotSDBError(SDBError expected, Runnable action) {
        try {
            action.run();
            fail();
        } catch (BaseException e) {
            assertNotEquals(expected.getErrorCode(), e.getErrorCode());
        }
    }

}

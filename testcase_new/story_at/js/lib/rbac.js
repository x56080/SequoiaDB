import("../lib/basic_operation/commlib.js");
import("../lib/main.js");

var test_role1_name = "test_role1";
var test_role2_name = "test_role2";
var test_role3_name = "test_role3";

var test_role1 = {
  Role: test_role1_name,
  Privileges: [
    {
      Resource: { cs: "test_rbac", cl: "test_rbac" },
      Actions: ["find"],
    },
  ],
  Roles: [],
};

var test_role2 = {
  Role: test_role2_name,
  Privileges: [
    {
      Resource: { cs: "test_rbac", cl: "test_rbac" },
      Actions: ["insert", "update"],
    },
  ],
  Roles: [test_role1_name],
};

var test_role3 = {
  Role: test_role3_name,
  Privileges: [
    {
      Resource: { cs: "test_rbac", cl: "test_rbac" },
      Actions: ["remove"],
    },
  ],
  Roles: [test_role2_name],
};

function cleanTestRoles(db) {
  try {
    db.dropRole(test_role1_name);
  } catch (e) {}
  try {
    db.dropRole(test_role2_name);
  } catch (e) {}
  try {
    db.dropRole(test_role3_name);
  } catch (e) {}
}

function prepareTestRoles(db) {
  cleanTestRoles(db);
  db.createRole(test_role1);
  db.createRole(test_role2);
  db.createRole(test_role3);
}

function checkRolesOfRole(role, expectedRoles, expectedInheritedRoles) {
  assert.equal(role.Roles.sort(), expectedRoles.sort());
  assert.equal(role.InheritedRoles.sort(), expectedInheritedRoles.sort());
}

function checkRolesOfRoleByName(db, roleName, expectedRoles, expectedInheritedRoles) {
  checkRolesOfRole(JSON.parse(db.getRole(roleName)), expectedRoles, expectedInheritedRoles);
}

function checkPrivilegesOfRole(role, expPrivileges, expInheritedPrivileges) {
  assert.equal(role.Privileges.sort(), expPrivileges.sort());
  assert.equal(role.InheritedPrivileges.sort(), expInheritedPrivileges.sort());
}

function checkPrivilegesOfRoleByName(db, roleName, expPrivileges, expInheritedPrivileges) {
  checkPrivilegesOfRole(JSON.parse(db.getRole(roleName, {ShowPrivileges: true})), expPrivileges, expInheritedPrivileges);
}

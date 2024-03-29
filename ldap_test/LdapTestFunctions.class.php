<?php

/**
 * @file
 * Utility functions for ldap simpletests.
 * @todo could be moved into LdapTestCase.class.php
 */

require_once 'ldap_servers.conf.inc';
require_once 'ldap_user.conf.inc';
require_once 'ldap_authentication.conf.inc';
require_once 'ldap_authorization.conf.inc';
/**
 *
 */
class LdapTestFunctions {

  public $data = [];
  /**
   * Data in ldap array format, but keyed on dn.
   */
  public $ldapData = [];
  public $csvTables = [];
  public $ldapTypeConf;

  /**
   *
   */
  public function __construct() {
    module_load_include('module', 'ldap_servers');
    $this->data['ldap_servers'] = ldap_test_ldap_servers_data();
    module_load_include('module', 'ldap_user');
    $this->data['ldap_user'] = ldap_test_ldap_user_data();
    module_load_include('module', 'ldap_authentication');
    $this->data['ldap_authorization'] = ldap_test_ldap_authorization_data();
    module_load_include('module', 'ldap_authorization');
    $this->data['ldap_authentication'] = ldap_test_ldap_authentication_data();
  }

  /**
   *
   */
  public function configureLdapServers($sids, $feetures = FALSE, $feature_name = NULL) {
    foreach ($sids as $i => $sid) {
      $current_sids[$sid] = $sid;
      config_set('ldap_test.settings', 'ldap_test_server__' . $sid, $this->data['ldap_servers'][$sid]);
    }
    config_set('ldap_test.settings', 'ldap_test_servers', $current_sids);
  }

  /**
   *
   */
  public function setFakeServerProperty($sid, $prop, $value) {
    $test_data = config_get('ldap_test.settings', 'ldap_test_server__' . $sid);
      if ($test_data === NULL) {
        $test_data = [];
      }
    $test_data['properties'][$prop] = $value;
    config_set('ldap_test.settings', 'ldap_test_server__' . $sid, $test_data);
  }

  /**
   *
   */
  public function setFakeServerUserAttribute($sid, $dn, $attr_name, $attr_value, $i = 0) {
    $attr_name = backdrop_strtolower($attr_name);
    $test_data = config_get('ldap_test.settings', 'ldap_test_server__' . $sid);
      if ($test_data === NULL) {
        $test_data = [];
      }

    $test_data['entries'][$dn][$attr_name][$i] = $attr_value;
    $count_set = (int) isset($test_data['entries'][$dn][$attr_name]['count']);
    // don't count the 'count'.
    $test_data['entries'][$dn][$attr_name]['count'] = count($test_data['entries'][$dn][$attr_name]) - $count_set;

    $test_data['ldap'][$dn][$attr_name][$i] = $attr_value;
    $count_set = (int) isset($test_data['ldap'][$dn][$attr_name]['count']);
    // don't count the 'count'.
    $test_data['ldap'][$dn][$attr_name]['count'] = count($test_data['ldap'][$dn][$attr_name]) - $count_set;
    config_set('ldap_test.settings', 'ldap_test_server__' . $sid, $test_data);
    // Clear server cache;.
    $ldap_server = ldap_servers_get_servers($sid, NULL, TRUE, TRUE);
  }

  /**
   *
   */
  public function configureLdapAuthentication($ldap_authentication_test_conf_id, $sids) {
    module_load_include('php', 'ldap_authentication', 'LdapAuthenticationConfAdmin.class');
    $options = $this->data['ldap_authentication'][$ldap_authentication_test_conf_id];
    foreach ($sids as $i => $sid) {
      $options['sids'][$sid] = $sid;
    }
    $ldapServerAdmin = new LdapAuthenticationConfAdmin();
    foreach ($ldapServerAdmin->saveable as $prop_name) {
      if (isset($options[$prop_name])) {
        $ldapServerAdmin->{$prop_name} = $options[$prop_name];
      }
    }
    $ldapServerAdmin->save();
  }

  /**
   *
   */
  public function configureLdapUser($ldap_user_test_conf_id) {
    module_load_include('php', 'ldap_user', 'LdapUserConfAdmin.class');
    $ldapUserConfAdmin = new LdapUserConfAdmin();
    $options = $this->data['ldap_user'][$ldap_user_test_conf_id];
    foreach ($ldapUserConfAdmin->saveable as $prop_name) {
      if (isset($options[$prop_name])) {
        $ldapUserConfAdmin->{$prop_name} = $options[$prop_name];
      }
    }
    $ldapUserConfAdmin->save();
  }

  /**
   *
   */
  public function prepConsumerConf($consumer_confs) {
    // Create consumer authorization configuration.
    foreach ($consumer_confs as $consumer_type => $consumer_conf) {
      $consumer_obj = ldap_authorization_get_consumer_object($consumer_type);
      $consumer_conf_admin = new LdapAuthorizationConsumerConfAdmin($consumer_obj, TRUE);
      foreach ($consumer_conf as $property_name => $property_value) {
        $consumer_conf_admin->{$property_name} = $property_value;
      }
      foreach ($consumer_conf_admin->mappings as $i => $mapping) {
        $mappings = $consumer_obj->normalizeMappings(
          [
            [$mapping['from'], $mapping['user_entered']],
          ],
           FALSE);
        $consumer_conf_admin->mappings[$i] = $mappings[0];
      }
      $consumer_conf_admin->save();
    }
  }

  /**
   *
   */
  public function ldapUserIsAuthmapped($username) {
    $authmaps = ldap_user_user_get_authmaps($username);
    return ($authmaps && in_array('ldap_user', array_keys($authmaps)));
  }

  /**
   *
   */
  public function backdropLdapUpdateUser($edit = [], $ldap_authenticated = FALSE, $user = NULL) {
    if (count($edit)) {
      if (!empty($user->data)) {
        $edit['data'] = !empty($edit['data']) ? array_merge($user->data, $edit['data']) : $user->data;
      }
      foreach ($edit as $key => $value) {
        $user->{$key} = $value;
      }
      $user->save();
    }
    if ($ldap_authenticated) {
      ldap_user_user_set_authmaps($user, ['authname_ldap_user' => $user->name]);
    }
    return $user;
  }

  /**
   * From https://www.midwesternmac.com/blogs/jeff-geerling/programmatically-adding-roles.
   */
  public function removeRoleFromUser($user, $role_name) {

    if (is_numeric($user)) {
      $user = user_load($user);
    }
    $roles_by_name = array_flip(user_roles());
    $key = array_search($roles_by_name[$role_name], $user->roles);
    if ($key == TRUE) {
      // Get the rid from the roles table.
      $roles = user_roles(TRUE);
      $rid = array_search($role_name, $roles);
      if ($rid != FALSE) {
        // Make a copy of the roles array, without the deleted one.
        $new_roles = [];
        foreach ($user->roles as $id => $name) {
          if ($name != $rid) {
            $new_roles[$name] = $name;
          }
        }
        $user->roles = $new_roles;
        user_save($user);
      }
    }
  }

  /**
   *
   */
  public function userByNameFlushingCache($name) {
    $user = user_load_by_name($name);
    // Clear user cache.
    $users = user_load_multiple([$user->uid], [], TRUE);
    $user = $users[$user->uid];
    return $user;
  }

  /**
   * Set variable with fake test data.
   *
   * @param string $test_ldap_id
   *   eg. 'hogwarts'.
   * @param string $test_ldap_type
   *   e.g. openLdap, openLdapTest1, etc.
   *
   * @parma string $sid where fake data is stored. e.g. 'default',
   */
  public function populateFakeLdapServerData($test_ldap_id, $sid = 'default') {

    // Read csvs into key/value array
    // create fake ldap data array.
    $clones = empty($this->data['ldap_servers'][$sid]['clones']) ? FALSE : $this->data['ldap_servers'][$sid]['clones'];
    $server_properties = $this->data['ldap_servers'][$sid]['properties'];
    $this->getCsvLdapData($test_ldap_id);
    foreach ($this->csvTables['users'] as $guid => $user) {
      $dn = 'cn=' . $user['cn'] . ',' . $this->csvTables['conf'][$test_ldap_id]['userbasedn'];
      $this->csvTables['users'][$guid]['dn'] = $dn;
      $attributes = $this->generateUserLDAPAttributes($test_ldap_id, $user);
      $this->addLDAPUserToLDAPArraysFromAttributes(
        $user,
        $sid,
        $dn,
        $attributes,
        $server_properties['ldap_type'],
        $server_properties['user_attr']
      );
    }

    if ($clones) {
      $clonable_user = $this->csvTables['users'][101];
      for ($i = 0; $i < $clones; $i++) {
        $user = $clonable_user;
        $cn = "clone" . $i;
        $dn = 'cn=' . $cn . ',' . $this->csvTables['conf'][$test_ldap_id]['userbasedn'];
        $user['cn'] = $cn;
        $user['dn'] = $dn;
        $user['uid'] = 20 + $i;
        $user['guid'] = 120 + $i;
        $user['lname'] = $user['lname'] . "_$i";
        $attributes = $this->generateUserLDAPAttributes($test_ldap_id, $user);
        $this->addLDAPUserToLDAPArraysFromAttributes(
          $user,
          $sid,
          $dn,
          $attributes,
          $server_properties['ldap_type'],
          $server_properties['user_attr']
        );
      }
    }

    foreach ($this->csvTables['groups'] as $guid => $group) {
      $dn = 'cn=' . $group['cn'] . ',' . $this->csvTables['conf'][$test_ldap_id]['groupbasedn'];
      $this->csvTables['groups'][$guid]['dn'] = $dn;
      $attributes = [
        'cn' => [
          0 => $group['cn'],
          'count' => 1,
        ],
        'gid' => [
          0 => $group['gid'],
          'count' => 1,
        ],
        'guid' => [
          0 => $guid,
          'count' => 1,
        ],
      ];

      if ($server_properties['groupMembershipsAttr']) {
        $membershipAttr = $server_properties['groupMembershipsAttr'];
        foreach ($this->csvTables['memberships'] as $membership_id => $membership) {
          if ($membership['gid'] == $group['gid']) {
            $member_guid = $membership['member_guid'];
            if (isset($this->csvTables['users'][$member_guid])) {
              $member = $this->csvTables['users'][$member_guid];
            }
            elseif (isset($this->csvTables['groups'][$member_guid])) {
              $member = $this->csvTables['groups'][$member_guid];
            }
            if ($server_properties['groupMembershipsAttrMatchingUserAttr'] == 'dn') {
              $attributes[$server_properties['groupMembershipsAttr']][] = $member['dn'];
            }
            else {
              $attributes[$server_properties['groupMembershipsAttr']][] = $member['attr'][$membershipAttr][0];
            }
          }
        }
        $attributes[$membershipAttr]['count'] = count($attributes[$membershipAttr]);

      }
      // Need to figure out if memberOf type attribute is desired and populate it.
      $this->data['ldap_servers_by_guid'][$sid][$group['guid']]['attr'] = $attributes;
      $this->data['ldap_servers_by_guid'][$sid][$group['guid']]['dn'] = $dn;
      $this->data['ldap_servers'][$sid]['groups'][$dn]['attr'] = $attributes;
      $this->ldapData['ldap_servers'][$sid][$dn] = $attributes;

    }
    if ($server_properties['groupUserMembershipsAttrExists']) {
      $member_attr = $server_properties['groupUserMembershipsAttr'];
      foreach ($this->csvTables['memberships'] as $gid => $membership) {
        $group_dn = $this->data['ldap_servers_by_guid'][$sid][$membership['group_guid']]['dn'];
        $user_dn = $this->data['ldap_servers_by_guid'][$sid][$membership['member_guid']]['dn'];
        $this->ldapData['ldap_servers'][$sid][$user_dn][$member_attr][] = $group_dn;
        if (isset($this->ldapData['ldap_servers'][$sid][$user_dn][$member_attr]['count'])) {
          unset($this->ldapData['ldap_servers'][$sid][$user_dn][$member_attr]['count']);
        }
        $this->ldapData['ldap_servers'][$sid][$user_dn][$member_attr]['count'] =
        count($this->ldapData['ldap_servers'][$sid][$user_dn][$member_attr]);
      }
    }

    $this->data['ldap_servers'][$sid]['ldap'] = $this->ldapData['ldap_servers'][$sid];
    $this->data['ldap_servers'][$sid]['csv'] = $this->csvTables;
    config_set('ldap_test.settings', 'ldap_test_server__' . $sid, $this->data['ldap_servers'][$sid]);
    $current_sids = config_get('ldap_test.settings', 'ldap_test_servers');
    $current_sids[] = $sid;
    config_set('ldap_test.settings', 'ldap_test_servers', array_unique($current_sids));
  }

  /**
   *
   */
  public function generateUserLDAPAttributes($test_ldap_id, $user) {
    $attributes = [
      'cn' => [
        0 => $user['cn'],
        'count' => 1,
      ],
      'mail' => [
        0 => $user['cn'] . '@' . $this->csvTables['conf'][$test_ldap_id]['mailhostname'],
        'count' => 1,
      ],
      'uid' => [
        0 => $user['uid'],
        'count' => 1,
      ],
      'guid' => [
        0 => $user['guid'],
        'count' => 1,
      ],
      'sn' => [
        0 => $user['lname'],
        'count' => 1,
      ],
      'givenname' => [
        0 => $user['fname'],
        'count' => 1,
      ],
      'house' => [
        0 => $user['house'],
        'count' => 1,
      ],
      'department' => [
        0 => $user['department'],
        'count' => 1,
      ],
      'faculty' => [
        0 => (int) (boolean) $user['faculty'],
        'count' => 1,
      ],
      'staff' => [
        0 => (int) (boolean) $user['staff'],
        'count' => 1,
      ],
      'student' => [
        0 => (int) (boolean) $user['student'],
        'count' => 1,
      ],
      'gpa' => [
        0 => $user['gpa'],
        'count' => 1,
      ],
      'probation' => [
        0 => (int) (boolean) $user['probation'],
        'count' => 1,
      ],
      'password'  => [
        0 => 'goodpwd',
        'count' => 1,
      ],
    ];
    return $attributes;
  }

  /**
   *
   */
  public function addLDAPUserToLDAPArraysFromAttributes($user, $sid, $dn, $attributes, $ldap_type, $user_attr) {

    if ($ldap_type == 'activedirectory') {
      $attributes[$user_attr] = [0 => $user['cn'], 'count' => 1];
      $attributes['distinguishedname'] = [0 => $dn, 'count' => 1];
    }
    elseif ($ldap_type == 'openldap') {

    }

    $this->data['ldap_servers'][$sid]['users'][$dn]['attr'] = $attributes;
    $this->data['ldap_servers_by_guid'][$sid][$user['guid']]['attr'] = $attributes;
    $this->data['ldap_servers_by_guid'][$sid][$user['guid']]['dn'] = $dn;
    $this->ldapData['ldap_servers'][$sid][$dn] = $attributes;
    $this->ldapData['ldap_servers'][$sid][$dn]['count'] = count($attributes);
  }

  /**
   *
   */
  public function getCsvLdapData($test_ldap_id) {
    foreach (['groups', 'users', 'memberships', 'conf'] as $type) {
      $path = backdrop_get_path('module', 'ldap_test') . '/test_ldap/' . $test_ldap_id . '/' . $type . '.csv';
      $this->csvTables[$type] = $this->parseCsv($path);
    }
  }

  /**
   *
   */
  public function parseCsv($filepath) {
    $row = 1;
    $table = [];
    if (($handle = fopen($filepath, "r")) !== FALSE) {
      while (($data = fgetcsv($handle, 1000, ",")) !== FALSE) {
        if (count($data) > 1) {
          $table[] = $data;
        }
      }
      fclose($handle);
    }

    $table_associative = [];
    $headings = array_shift($table);
    foreach ($table as $i => $row) {
      $row_id = $row[0];
      foreach ($row as $j => $item) {
        $table_associative[$row_id][$headings[$j]] = $item;
      }
    }

    return $table_associative;

  }

}

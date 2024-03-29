<?php

/**
 * @file
 */

module_load_include('inc', 'ldap_servers', 'ldap_servers.functions');

/**
 * @file
 */

/**
 * Implements the LDAP server test page.
 *
 * @param $form_state
 *   A form state array.
 * @param $op
 *   An operatin - add or edit.
 * @param $sid
 *   A LDAP server ID.
 *
 * @return
 *   The form structure.
 */
function ldap_servers_test_form($form, &$form_state, $op = NULL, $sid = NULL) {

  $ldap_server = ldap_servers_get_servers($sid, 'all', TRUE);

  backdrop_set_title(t('Test LDAP Server Configuration: !server', ['!server' => $ldap_server->name]));

  $form['#prefix'] = t('This form tests an LDAP configuration to see if
    it can bind and basic user and group functions. It also shows token examples
    and a sample user. The only data this function will modify is the test LDAP group, which will be deleted and added');

  $variables = [
    'ldap_server' => $ldap_server,
    'actions' => FALSE,
    'type' => 'detail',
  ];

  $form['server_variables'] = [
    '#markup' => theme('ldap_servers_server', $variables),
  ];

  $form['sid'] = [
    '#type' => 'hidden',
    '#default_value' => $sid,
  ];

  $form['binding']['bindpw'] = [
    '#type' => 'password',
    '#title' => t('Password for non-anonymous search'),
    '#size' => 20,
    '#maxlength' => 255,
    '#description' => t('Leave empty to test with currently stored password.'),
  ];

  $form['testing_backdrop_username'] = [
    '#type' => 'textfield',
    '#title' => t('Testing Backdrop Username'),
    '#default_value' => $ldap_server->testingBackdropUsername,
    '#size' => 30,
    '#maxlength' => 255,
    '#description' => t('This is optional and used for testing this server\'s configuration against an actual username. The user need not exist in Backdrop and testing will not affect the user\'s LDAP or Backdrop Account.'),
  ];

  $form['testingBackdropUserDn'] = [
    '#type' => 'textfield',
    '#title' => t('Testing Backdrop DN'),
    '#default_value' => $ldap_server->testingBackdropUserDn,
    '#size' => 120,
    '#maxlength' => 255,
    '#description' => t('This is optional and used for testing this server\'s configuration against an actual username. The user need not exist in Backdrop and testing will not affect the user\'s LDAP or Backdrop Account.'),
  ];

  $form['grp_test_grp_dn'] = [
    '#type' => 'textfield',
    '#title' => t('Testing Group DN'),
    '#default_value' => $ldap_server->groupTestGroupDn,
    '#size' => 120,
    '#maxlength' => 255,
    '#description' => t('This is optional and used for testing this server\'s group configuration.'),
  ];

  $form['grp_test_grp_dn_writeable'] = [
    '#type' => 'textfield',
    '#title' => t('Testing Group DN that is writeable. Warning!  In test, this group will be deleted, created, have members added to it!'),
    '#default_value' => $ldap_server->groupTestGroupDnWriteable,
    '#size' => 120,
    '#maxlength' => 255,
    '#description' => t('This is optional and used for testing this server\'s group configuration.'),
  ];

  $form['grp_test_grp_required_member'] = [
    '#type' => 'checkbox',
    '#title' => t("LDAP Group Entry Attribute Holding User's DN, CN is a required attribute of the group."),
    '#description' => t('LDAP group must have at least one member. Empty group is not allowed.'),
  ];

  if ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_ANON_USER) {
    $form['testing_backdrop_userpw'] = [
      '#type' => 'password',
      '#title' => t('Testing Backdrop User Password'),
      '#size' => 30,
      '#maxlength' => 255,
      '#description' => t('This is optional and used for testing this server\'s configuration against the username above.'),
    ];
  }

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => 'Test',
    '#weight' => 100,
  ];

  if (!empty($form_state['ldap_server_test_data'])) {
    $test_data = $form_state['ldap_server_test_data'];

    if (isset($test_data['username']) && isset($test_data['ldap_user'])) {
      $form['#prefix'] = theme('ldap_server_ldap_entry_table',
        [
          'entry' => isset($test_data['ldap_user']['attr']) ? $test_data['ldap_user']['attr'] : '',
          'username' => $test_data['username'],
          'dn' => isset($test_data['ldap_user']['dn']) ? $test_data['ldap_user']['dn'] : '',
        ]);
    }

    $titles = [
      'basic' => 'Test Results',
      'group1' => 'Group Create, Delete, Add Member, Remove Member Tests',
      'group2' => 'User Group Membership Functions Test',
      'tokens' => 'User Token Samples',
      'groupfromDN' => 'Groups Derived From User DN',
    ];
    foreach ($test_data['results_tables'] as $table_name => $table_data) {
      $form['#prefix'] .= '<h2>' . $titles[$table_name] . '</h2>' . theme('table', ['header' => ['Test', 'Result'], 'rows' => $table_data]);
    }

    if (function_exists('dpm') && !empty($test_data['username'])) {
      $user_name = $test_data['username'];
      if ($user = user_load_by_name($user_name)) {
        dpm("Corresponding Backdrop user object for: $user_name");
        dpm($user);
        if (function_exists('entity_load')) {
          $user_entity = entity_load('user', $user->uid);
          dpm("Backdrop user entity for: $user_name");
          dpm($user_entity);
        }
        if (isset($test_data['group_entry'][0])) {
          dpm("Test Group LDAP Entry");
          dpm($test_data['group_entry'][0]);
        }
      }
    }
  }
  return $form;
}

/**
 *
 */
function ldap_servers_string_binary_check($input) {
  if (preg_match('~[^\x20-\x7E\t\r\n]~', $input) > 0) {
    $truncatedString = truncate_utf8($input, 200);
    return t('Binary (excerpt): @excerpt', ['@excerpt' => $truncatedString]);
  }
  else {
    return $input;
  }
}

/**
 * Validate hook for the LDAP server form.
 */
function ldap_servers_test_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  if (!$values['sid']) {
    form_set_error(NULL, t('No server id found in form'));
  }
  elseif (!$ldap_server = ldap_servers_get_servers($values['sid'], 'all', TRUE)) {
    form_set_error(NULL, t('Failed to create server object for server with server id=%sid', ['%sid' => $values['sid']]));
  }
}

/**
 * Submit hook for the LDAP server form.
 */
function ldap_servers_test_form_submit($form, &$form_state) {

  // Pass data back to form builder.
  $form_state['rebuild'] = TRUE;

  ldap_servers_module_load_include('inc', 'ldap_servers', 'ldap_servers.functions');
  $errors = FALSE;
  $has_errors = FALSE;
  $values = $form_state['values'];
  $sid = $values['sid'];
  $ldap_server = ldap_servers_get_servers($sid, 'all', TRUE);

  $results = [];
  $results_tables = [];
  if ($values['bindpw']) {
    $bindpw = $values['bindpw'];
    $bindpw_type = t('entered in form.');
  }
  else {
    $bindpw = NULL;
    $bindpw_type = t('stored in configuration');
  }

  if ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT) {
    $results_tables['basic'][] = [
      t('Testing method: Binding with DN for non-anonymous search (%bind_dn). Using password ',
      ['%bind_dn' => $ldap_server->binddn]) . ' ' . $bindpw_type,
    ];
  }
  else {
    $results_tables['basic'][] = [t('Testing method: Binding with null DN for anonymous search.')];
  }

  if (@$values['grp_test_grp_dn_writeable'] && @$values['grp_test_grp_dn']) {
    $user_test_dn = @$values['grp_test_grp_dn'];
    $group_create_test_dn = $values['grp_test_grp_dn_writeable'];
    $group_create_test_attr = [
      'objectClass' => [$ldap_server->groupObjectClass, 'top'],
    ];

    $min_members = 0;
    if ($values['grp_test_grp_required_member']) {
      // Create a new member.
      $required_member = explode('=', $user_test_dn);
      $required_member[1] = 'rm'. $required_member[1];
      $required_member = implode('=', $required_member);
      // Add member to group, because group must have at least one member.
      $group_create_test_attr[$ldap_server->groupMembershipsAttr] = $required_member;
      $min_members = 1;
    }

    // 1. delete test group if it exists.
    if ($ldap_server->dnExists($group_create_test_dn, 'ldap_entry', ['cn', 'member'])) {
      $result = $ldap_server->groupRemoveGroup($group_create_test_dn, FALSE);
    }

    $group_exists = $ldap_server->dnExists($group_create_test_dn, 'ldap_entry', ['cn', 'member']);
    $result = ($group_exists === FALSE) ? "PASS" : "FAIL";
    $results_tables['group1'][] = ["1. Starting test without group: $group_create_test_dn doesn't exist", $result];

    // 2. make sure call to members in empty group returns false.
    $result = $ldap_server->groupAllMembers($group_create_test_dn);
    $result = ($result === FALSE) ? "PASS" : 'FAIL';
    $results_tables['group1'][] = ["2. LdapServer::groupAllMembers($group_create_test_dn) call on nonexistent group returns FALSE", $result];

    // 3. add group.
    $result = $ldap_server->groupAddGroup($group_create_test_dn, $group_create_test_attr);
    $result = ($result) ? "PASS" : 'FAIL';
    $attr = serialize($group_create_test_attr);
    $results_tables['group1'][] = ["3. LdapServer::groupAddGroup($group_create_test_dn, $attr)", $result];

    // 4. call to all members in an empty group returns emtpy array, not FALSE.
    $result = $ldap_server->groupAllMembers($group_create_test_dn);
    $result = (is_array($result) && count($result) == $min_members) ? 'PASS' : 'FAIL';
    $results_tables['group1'][] = ["4. LdapServer::groupAllMembers($group_create_test_dn) returns empty array for empty group ", $result];

    // 5. add member to group.
    $ldap_server->groupAddMember($group_create_test_dn, $user_test_dn);
    $result = $ldap_server->groupAllMembers($group_create_test_dn);
    $result = (is_array($result) && count($result) == $min_members + 1) ? 'PASS' : 'FAIL';
    $results_tables['group1'][] = ["5. LdapServer::groupAddMember($group_create_test_dn, $user_test_dn)", $result];

    // 6. try to remove group with member in it.
    $only_if_group_empty = TRUE;
    $result = $ldap_server->groupRemoveGroup($group_create_test_dn, $only_if_group_empty);
    $result = ($result) ? 'FAIL' : 'PASS';
    $results_tables['group1'][] = ["6. LdapServer::groupRemoveGroup($group_create_test_dn, $only_if_group_empty)", $result];

    // 7. remove group member.
    $result = $ldap_server->groupRemoveMember($group_create_test_dn, $user_test_dn);
    $result = $ldap_server->groupAllMembers($group_create_test_dn);
    $result = (is_array($result) && count($result) == $min_members) ? 'PASS' : 'FAIL';
    $results_tables['group1'][] = ["7. LdapServer::groupRemoveMember($group_create_test_dn, $user_test_dn)", $result];

    // 8. remove group.
    $only_if_group_empty = FALSE;
    $result = $ldap_server->groupRemoveGroup($group_create_test_dn, $only_if_group_empty);
    $result = ($ldap_server->dnExists($group_create_test_dn, 'ldap_entry', ['cn', 'member'])) ? "FAIL" : 'PASS';
    $results_tables['group1'][] = ["8. LdapServer::groupRemoveGroup($group_create_test_dn, $only_if_group_empty)", $result];
  }

  // Connect to ldap.
  list($has_errors, $more_results) = ldap_servers_test_binding_credentials($ldap_server, $bindpw, $results_tables);
  $results = array_merge($results, $more_results);
  if ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_ANON_USER) {
    list($has_errors, $more_results, $ldap_user) = ldap_servers_test_user_mapping($values['testing_backdrop_username'], $ldap_server);
    $results = array_merge($results, $more_results);
    if (!$has_errors) {
      $mapping[] = "dn = " . $ldap_user['dn'];
      foreach ($ldap_user['attr'] as $key => $value) {
        if (is_array($value)) {
          $mapping[] = "$key = " . $value[0];
        }
      }
      $results_tables['basic'][] = [theme('item_list', ['items' => $mapping, 'title' => t('Attributes available to anonymous search', ['%bind_dn' => $ldap_server->binddn]), 'type' => 'ul', 'attributes' => [], 'wrapper_attributes' => [], 'empty' => ''])];
    }
    $results_tables['basic'][] = [
      t('Testing method: Binding with DN (%bind_dn). Using supplied password ',
      ['%bind_dn' => $ldap_user['dn']]),
    ];
    $result = $ldap_server->bind($ldap_user['dn'], $values['testing_backdrop_userpw'], FALSE);
    if ($result == LDAP_SUCCESS) {
      $results_tables['basic'][] = [t('Successfully bound to server'), 'PASS'];
    }
    else {
      $results_tables['basic'][] = [t('Failed to bind to server. ldap error #') . $result . ' ' . $ldap_server->errorMsg('ldap'), 'FAIL'];
    }
  }

  if (!$has_errors && isset($values['grp_test_grp_dn'])) {
    $group_dn = $values['grp_test_grp_dn'];

    $result = @ldap_read($ldap_server->connection, $group_dn, 'objectClass=*');
    if (is_array($result)) {
      $group_entry = ldap_get_entries($ldap_server->connection, $result);
    }
    $user = isset($values['testing_backdrop_username']) ? $values['testing_backdrop_username'] : NULL;

    // FALSE.
    foreach ([FALSE, TRUE] as $nested) {
      $nested_display = ($nested) ? 'Yes' : 'No';
      if ($user) {
        // This is the parent function that will call FromUserAttr or FromEntry.
        $memberships = $ldap_server->groupMembershipsFromUser($user, 'group_dns', $nested);
        $result = theme('item_list', ['items' => $memberships, 'type' => 'ul']);
        $results_tables['group2'][] = ["ldap_server->groupMembershipsFromUser($user, 'group_dns', nested=$nested_display)<br/>count=" . count($memberships), $result];

        $result = ($ldap_server->groupIsMember($group_dn, $user, $nested)) ? 'Yes' : 'No';
        $group_results[] = ["ldap_server->groupIsMember($group_dn, $user, nested=$nested_display)", $result];

        if ($ldap_server->groupUserMembershipsConfigured) {
          $groupusermembershipsfromuserattr = $ldap_server->groupUserMembershipsFromUserAttr($user, $nested);
          $count = count($groupusermembershipsfromuserattr);
          $result = theme('item_list', ['items' => $groupusermembershipsfromuserattr, 'type' => 'ul']);
        }
        else {
          $groupusermembershipsfromuserattr = [];
          $result = "'A user LDAP attribute such as memberOf exists that contains a list of their group' is not configured.";
        }
        $results_tables['group2'][] = [
          "ldap_server->groupUserMembershipsFromUserAttr($user, nested=$nested_display)<br/> count=" .
          count($groupusermembershipsfromuserattr), $result,
        ];

        if ($ldap_server->groupGroupEntryMembershipsConfigured) {
          $groupusermembershipsfromentry = $ldap_server->groupUserMembershipsFromEntry($user, $nested);
          $result = theme('item_list', ['items' => $groupusermembershipsfromentry, 'type' => 'ul']);
        }
        else {
          $groupusermembershipsfromentry = [];
          $result = "Groups by entry not configured.";
        }
        $results_tables['group2'][] = [
          "ldap_server->groupUserMembershipsFromEntry($user, nested=$nested_display)<br/>count=" .
          count($groupusermembershipsfromentry), $result,
        ];

        if (count($groupusermembershipsfromentry) && count($groupusermembershipsfromuserattr)) {
          $diff1 = array_diff($groupusermembershipsfromuserattr, $groupusermembershipsfromentry);
          $diff2 = array_diff($groupusermembershipsfromentry, $groupusermembershipsfromuserattr);
          $result1 = theme('item_list', ['items' => $diff1, 'type' => 'ul']);
          $result2 = theme('item_list', ['items' => $diff2, 'type' => 'ul']);
          $results_tables['group2'][] = ["groupUserMembershipsFromEntry and FromUserAttr Diff)", $result1];
          $results_tables['group2'][] = ["FromUserAttr and groupUserMembershipsFromEntry Diff)", $result2];
        }
      }
    }

    if ($groups_from_dn = $ldap_server->groupUserMembershipsFromDn($user)) {
      $results_tables['groupfromDN'][] = ["Groups from DN", theme('item_list', ['items' => $groups_from_dn, 'type' => 'ul'])];
    }

  }

  list($has_errors, $more_results, $ldap_user) = ldap_servers_test_user_mapping($values['testing_backdrop_username'], $ldap_server);

  $tokens = ($ldap_user && isset($ldap_user['attr'])) ? ldap_servers_token_tokenize_entry($ldap_user['attr'], 'all') : [];
  foreach ($tokens as $key => $value) {
    $results_tables['tokens'][] = [$key, ldap_servers_string_binary_check($value)];
  }
  $form_state['ldap_server_test_data'] = [
    'username' => $values['testing_backdrop_username'],
    'results_tables' => $results_tables,
  ];

  if (isset($ldap_user)) {
    $form_state['ldap_server_test_data']['ldap_user'] = $ldap_user;
  }

  if (isset($group_entry)) {
    $form_state['ldap_server_test_data']['group_entry'] = $group_entry;
  }

}

/**
 * Submit hook for the LDAP server form.
 */
function ldap_servers_test_binding_credentials(&$ldap_server, $bindpw, &$results_tables) {

  $errors = FALSE;
  $results = [];

  $ldap_result = $ldap_server->connect();
  if ($ldap_result != LDAP_SUCCESS) {
    $results_tables['basic'][] = [
      t('Failed to connect to LDAP server. See watchdog error logs for details.') .
      $ldap_server->errorMsg('ldap'),
    ];
    $errors = TRUE;
  }

  if (!$errors) {
    $bind_result = $ldap_server->bind($ldap_server->binddn, $bindpw, FALSE);
    if ($bind_result == LDAP_SUCCESS) {
      $results_tables['basic'][] = [t('Successfully bound to server'), 'PASS'];
    }
    else {
      $results_tables['basic'][] = [t('Failed to bind to server. ldap error #') . $bind_result . ' ' . $ldap_server->errorMsg('ldap'), 'FAIL'];
      $errors = TRUE;
    }
  }
  return [$errors, $results];
}

/**
 *
 */
function ldap_servers_test_user_mapping($backdrop_username, &$ldap_server, $ldap_context = NULL) {

  $ldap_user = $ldap_server->userUserNameToExistingLdapEntry($backdrop_username, $ldap_context);
  $errors = FALSE;
  if (!$ldap_user) {
    $results[] = t('Failed to find test user %username by searching on  %user_attr = %username.',
      [
        '%username' => $backdrop_username,
        '%user_attr' => $ldap_server->user_attr,
      ]
      )
      . ' ' . t('Error Message:') . ' ' . $ldap_server->errorMsg('ldap');
    $errors = TRUE;
  }
  else {
    $results[] = t('Found test user %username by searching on  %user_attr = %username.',
      ['%username' => $backdrop_username, '%user_attr' => $ldap_server->user_attr]);
  }
  return [$errors, $results, $ldap_user];
}

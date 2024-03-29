<?php

/**
 * @file
 * Ldap_authentication helper functions.
 */

/**
 * Helper function for ldap_authn_form_user_login_block_alter and ldap_authn_form_user_login_alter.
 *
 * @todo if form is being generated on non https and is set in preferences, set warning and end form development
 */
function _ldap_authentication_login_form_alter(&$form, &$form_state, $form_id) {

  if (!$auth_conf = ldap_authentication_get_valid_conf()) {
    return;
  }
  elseif (!$auth_conf->hasEnabledAuthenticationServers()) {
    return;
  }

  /**
   *
   * add validate function to test for ldap authentication
   * should be placed after user_login_authenticate_validate
   * 1. user_login_name_validate
   * 2. user_login_authenticate_validate
   * 3. external authentication validate functions
   * 4. user_login_final_validate
   *
   * as articulated above user_login_default_validators() in user.module
   *
   * without any other external authentication modules, this array will start out as:
   *    array('user_login_name_validate', 'user_login_authenticate_validate', 'user_login_final_validate')
   */

  if (@in_array('user_login_authenticate_validate', $form['#validate']) && $auth_conf->authenticationMode) {
    $key = array_search('user_login_authenticate_validate', $form['#validate']);
    $form['#validate'][$key] = 'ldap_authentication_core_override_user_login_authenticate_validate';
    array_splice($form['#validate'], $key + 1, 0, 'ldap_authentication_user_login_authenticate_validate');
  }

  if ($form_id == 'user_login_block') {
    $user_register = config_get('system.core', 'user_register');
    if ($user_register === NULL) {
      $user_register = USER_REGISTER_VISITORS_ADMINISTRATIVE_APPROVAL;
    }
    $vars = [
      'show_reset_pwd' => ldap_authentication_show_reset_pwd(),
      'auth_conf' => $auth_conf,
    ];

    $form['links']['#markup'] = theme('ldap_authentication_user_login_block_links', $vars);
  }

  // Add help information for entering in username/password.
  $auth_conf = ldap_authentication_get_valid_conf();
  if ($auth_conf) {
    if (isset($auth_conf->loginUIUsernameTxt)) {
      $form['name']['#description'] = t($auth_conf->loginUIUsernameTxt);
    }
    if (isset($auth_conf->loginUIPasswordTxt)) {
      $form['pass']['#description'] = t($auth_conf->loginUIPasswordTxt);
    }
    if ($auth_conf->templateUsageRedirectOnLogin) {
      $form['#submit'][] = 'ldap_authentication_check_for_email_template';
    }
  }
}

/**
 * Alter user editing form (profile form) based on ldap authentication configuration.
 *
 * @param array $form
 *   array from user profile.
 * @param array $form_state
 *   from user profile.
 *
 * @return NULL (alters $form by reference)
 */
function _ldap_authentication_form_user_profile_form_alter(&$form, $form_state) {
  // Keep in mind admin may be editing another users profile form. Don't assume current global $user.
  $auth_conf = ldap_authentication_get_valid_conf();
  if ($auth_conf && ldap_authentication_ldap_authenticated($form['#user'])) {
    if ($auth_conf->emailOption == LDAP_AUTHENTICATION_EMAIL_FIELD_REMOVE) {
      $form['account']['mail']['#access'] = FALSE;
    }
    elseif ($auth_conf->emailOption == LDAP_AUTHENTICATION_EMAIL_FIELD_DISABLE) {
      $form['account']['mail']['#disabled'] = TRUE;
      $form['account']['mail']['#description'] = t('This email address is automatically set and may not be changed.');
    }
    elseif ($auth_conf->emailOption == LDAP_AUTHENTICATION_EMAIL_FIELD_ALLOW) {
      // Email field is functional.
    }

    if (!ldap_authentication_show_reset_pwd($form['#user'])) {
      /**  If passwordOption = LDAP_AUTHENTICATION_PASSWORD_FIELD_HIDE then don't show the password fields,
       * otherwise show the fields but in a disabled state.
       */
      switch ($auth_conf->passwordOption) {

        case LDAP_AUTHENTICATION_PASSWORD_FIELD_HIDE:
          $form['account']['current_pass']['#access'] = FALSE;
          $form['account']['pass']['#access'] = FALSE;
          break;

        case LDAP_AUTHENTICATION_PASSWORD_FIELD_SHOW:
          // Show in a disabled state since ldap_authentication_show_reset_pwd() has returned FALSE.
          $form['account']['current_pass']['#disabled'] = TRUE;
          if ($auth_conf->ldapUserHelpLinkUrl) {
            $form['account']['current_pass']['#description'] = l(t($auth_conf->ldapUserHelpLinkText), $auth_conf->ldapUserHelpLinkUrl);
          }
          else {
            $form['account']['current_pass']['#description'] = t('The password cannot be changed using this website');
          }
          $form['account']['pass']['#disabled'] = TRUE;
          break;
      }
    }
  }
}

/**
 * Replaces the email address in $ldap_user with one from the template in
 * $auth_conf.
 *
 * @param array $ldap_user
 *   LDAP user entry.
 * @param LdapAuthenticationConf $auth_conf
 *   LDAP authentication configuration class.
 */
function _ldap_authentication_replace_user_email(&$ldap_user, $auth_conf, $tokens) {
  // Fallback template in case one was not specified.
  $template = '@username@localhost';
  if (!empty($auth_conf->emailTemplate)) {
    $template = $auth_conf->emailTemplate;
  }
  $ldap_user['mail'] = format_string($template, $tokens);
}

/**
 * User form validation will take care of username, pwd fields
 * this function validates ldap authentication specific.
 *
 * @param array $form_state
 *   array from user logon form.
 *
 * @return null, but success or failure is indicated by:
 *   -- form_set_error() to invalidate authentication process
 *   -- setting $form_state['uid'] to indicate successful authentication
 */
function _ldap_authentication_user_login_authenticate_validate(&$form_state, $return_user) {

  // Check if Flood control was triggered; if so, don't authenticate.
  if (isset($form_state['flood_control_triggered'])) {
    return;
  }

  $detailed_watchdog_log = (module_exists('ldap_help')) ? config_get('ldap_help.settings', 'ldap_help_watchdog_detail') : 0;

  // Default to name.
  $entered_name = $form_state['values']['name'];
  $authname_backdrop_property = $form_field_name = 'name';

  // Email registration module populates name even though user entered email.
  if (!empty($form_state['values']['email'])) {
    $entered_name = $form_state['values']['email'];
    $authname_backdrop_property = 'mail';
    $form_field_name = 'email';
  }

  // $authname is the name the user is authenticated with from the logon form // patch 1599632.
  $authname = $entered_name;

  if (empty($form_state['values']['pass']) || empty($form_state['values'][$form_field_name])) {
    return FALSE;
  }
  /*
   * If a fake form state was passed into this function from
   * _ldap_authentication_user_login_sso(), there will be a value outside of the
   * form_state[values] array to let us know that we are not authenticating with
   * a password, but instead just looking up a username/dn in LDAP since the web
   * server already authenticated the user.
   */
  $sso_login = (isset($form_state['sso_login']) && $form_state['sso_login']) ? TRUE : FALSE;

  // Patch 1599632.
  $watchdog_tokens = ['%username' => $authname, '%authname' => $authname];
  if ($detailed_watchdog_log) {
    watchdog('ldap_authentication', '%username : Beginning authentification....', $watchdog_tokens, WATCHDOG_DEBUG);
  }

  if (!$auth_conf = ldap_authentication_get_valid_conf()) {
    watchdog('ldap_authentication', 'Failed to get valid ldap authentication configuration.', [], WATCHDOG_ERROR);
    form_set_error('name', 'Server Error: Failed to get valid ldap authentication configuration.');
    return;
  }

  /**
  * I. Test for previous module authentication success.
  *
  * if already succeeded at authentication, $form_state['uid'] will be set by other authentication module.
  * - if LDAP Mixed mode is set, return and don't disrupt authentication process
  * - otherwise override other authenication by setting $form_state['uid'] = NULL
  */
  if (isset($form_state['uid']) && is_numeric($form_state['uid'])) {
    if ($auth_conf->authenticationMode == LDAP_AUTHENTICATION_MIXED || $form_state['uid'] == 1) {
      if ($detailed_watchdog_log) {
        watchdog('ldap_authentication', '%username : Previously authenticated in mixed mode or uid=1', $watchdog_tokens, WATCHDOG_DEBUG);
      }
      // Already passed a previous module's authentication validation.
      return;
    }
    elseif ($auth_conf->authenticationMode == LDAP_AUTHENTICATION_EXCLUSIVE) {
      if ($detailed_watchdog_log) {
        watchdog('ldap_authentication', '%username : Previously authenticated in exclusive mode or uid is not 1. Clear uid
        in form_state and attempt ldap authentication.', $watchdog_tokens, WATCHDOG_DEBUG);
      }
      // Passed previous authentication, but only ldap should be used so override.
      $form_state['uid'] = NULL;
    }
  }

  /**
   * II. Exit if no authentication servers.
   */
  if (!$auth_conf->hasEnabledAuthenticationServers()) {
    watchdog('ldap_authentication', 'No LDAP servers configured.', [], WATCHDOG_ERROR);
    form_set_error('name', 'Server Error:  No LDAP servers configured.');
    return;
  }

  /**
   * III. determine if corresponding backdrop account exists for $authname
   */
  $backdrop_account_is_authmapped = FALSE;
  list($backdrop_account, $backdrop_account_is_authmapped) = ldap_authentication_corresponding_backdrop_user($authname, $auth_conf, $watchdog_tokens);
  $backdrop_account_exists = is_object($backdrop_account);
  if ($backdrop_account_exists && $backdrop_account->uid == 1) {
    // User 1 is not allowed to ldap authenticate.
    return;
  }

  /**
   * IV. test credentials and if available get corresponding ldap user and ldap server
   */
  list($authentication_result, $ldap_user, $ldap_server_authenticated_on) = ldap_authentication_test_credentials($auth_conf, $sso_login, $authname, $form_state['values']['pass'], $watchdog_tokens);
  $params['account'] = $backdrop_account;
  backdrop_alter('ldap_entry', $ldap_user, $params);
  if ($authentication_result != LDAP_AUTHENTICATION_RESULT_SUCCESS) {
    ldap_authentication_fail_response($authentication_result, $auth_conf, $detailed_watchdog_log, $watchdog_tokens);
    return;
  }

  /**
   * IV.a Workaround for the provisioning server not always getting saved in
   *      the user object. So we save it in a session variable as a backup.
   */
  if ($ldap_server_authenticated_on) {
    $_SESSION[LDAP_USER_SESSION_PROV_SID] = $ldap_server_authenticated_on->sid;
  }

  /**
   * V. if account_name_attr is set, backdrop username is different than authname
   */
  if ($ldap_server_authenticated_on->account_name_attr != '') {
    $watchdog_tokens['%account_name_attr'] = $ldap_server_authenticated_on->account_name_attr;
    $backdrop_accountname = $ldap_user['attr'][ldap_server_massage_text($ldap_server_authenticated_on->account_name_attr, 'attr_name', LDAP_SERVER_MASSAGE_QUERY_ARRAY)][0];
    if (!$backdrop_accountname) {
      watchdog('ldap_authentication', 'Derived backdrop username from attribute %account_name_attr returned no username for authname %authname.', $watchdog_tokens, WATCHDOG_ERROR);
      return;
    }
  }
  else {
    $backdrop_accountname = $authname;
  }
  $watchdog_tokens['%backdrop_accountname'] = $backdrop_accountname;

  // @todo maybe we can add more tokens?
  $email_template_tokens = [
    '@username' => $backdrop_accountname,
  ];

  $email_template_used = FALSE;

  /**
   * Ensures that we respect the email template handling settings.
   */
  if (!empty($auth_conf->emailTemplate)) {
    switch ($auth_conf->emailTemplateHandling) {
      case LDAP_AUTHENTICATION_EMAIL_TEMPLATE_IF_EMPTY:
        if (!empty($ldap_user['mail'])) {
          break;
        }
        // Deliberate fallthrough.
      case LDAP_AUTHENTICATION_EMAIL_TEMPLATE_ALWAYS:
        _ldap_authentication_replace_user_email($ldap_user, $auth_conf, $email_template_tokens);
        if ($detailed_watchdog_log) {
          watchdog('ldap_authentication', 'Using template generated email for %username', $watchdog_tokens, WATCHDOG_DEBUG);
        }
        $email_template_used = TRUE;
        break;
    }
  }

  /**
   * VI. Find or create corresponding backdrop account and set authmaps
   *
   * at this point, the following are know:
   * - a corresponding ldap account has been found
   * - user's credentials tested against it and passed
   * - their backdrop accountname has been derived
   *
   */

  /**
   * VI.A: Backdrop account doesn't exist with $authname used to logon,
   *  but puid exists in another Backdrop account; this means username has changed
   *  and needs to be saved in Backdrop account
   *
   */
  if (!$backdrop_account_exists && $ldap_server_authenticated_on) {
    $puid = $ldap_server_authenticated_on->userPuidFromLdapEntry($ldap_user['attr']);
    if ($puid) {
      $backdrop_account = $ldap_server_authenticated_on->userUserEntityFromPuid($puid);
      if ($backdrop_account) {
        $backdrop_account_exists = TRUE;
        if ($backdrop_accountname == $authname) {
          $user_edit = [$authname_backdrop_property => $backdrop_accountname];
        }
        else {
          $user_edit = ['name' => $backdrop_accountname];
        }
        if (!empty($backdrop_account->data)) {
          $user_edit['data'] = !empty($user_edit['data']) ? array_merge($backdrop_account->data, $user_edit['data']) : $backdrop_account->data;
        }
        foreach ($user_edit as $key => $value) {
          $backdrop_account->{$key} = $value;
        }
        $backdrop_account->save();
        ldap_user_user_set_authmaps($backdrop_account, ["authname_ldap_user" => $authname]);
        $backdrop_account_is_authmapped = TRUE;
      }
    }
  }

  /**
   * VI.B: existing Backdrop account but not authmapped to ldap modules,
   * ldap authmap or disallow.
   */
  // Account already exists.
  if ($backdrop_account_exists && !$backdrop_account_is_authmapped) {
    if ($auth_conf->ldapUser->loginConflictResolve == LDAP_USER_CONFLICT_LOG) {
      if ($account_with_same_email = user_load_by_mail($ldap_user['mail'])) {
        $watchdog_tokens['%conflict_name'] = $account_with_same_email->name;
        watchdog('ldap_authentication', 'LDAP user with DN %dn has a naming conflict with a local backdrop user %conflict_name', $watchdog_tokens, WATCHDOG_ERROR);
      }
      backdrop_set_message(t('Another user already exists in the system with the same login name. You should contact the system administrator in order to solve this conflict.'), 'error');
      return;
    }
    // LDAP_authen.AC.disallow.ldap.backdrop.
    else {
      // Add ldap_authentication authmap to user. account name is fine here, though cn could be used.
      ldap_user_user_set_authmaps($backdrop_account, ['authname_ldap_user' => $authname]);
      $backdrop_account_is_authmapped = TRUE;
      if ($detailed_watchdog_log) {
        watchdog('ldap_authentication', 'set authmap for %username authname_ldap_user', $watchdog_tokens, WATCHDOG_DEBUG);
      }
    }
  }

  /**
   * VI.C: existing Backdrop account with incorrect email.
   * Fix email if appropriate.
   */
  if ((!($auth_conf->templateUsageNeverUpdate && $email_template_used)) &&
      $backdrop_account_exists &&
      $backdrop_account->mail != $ldap_user['mail'] &&
      (
        $auth_conf->emailUpdate == LDAP_AUTHENTICATION_EMAIL_UPDATE_ON_LDAP_CHANGE_ENABLE_NOTIFY ||
        $auth_conf->emailUpdate == LDAP_AUTHENTICATION_EMAIL_UPDATE_ON_LDAP_CHANGE_ENABLE
      )) {
    $user_edit = ['mail' => $ldap_user['mail']];

    $watchdog_tokens['%username'] = $backdrop_account->name;
    if (!empty($backdrop_account->data)) {
      $user_edit['data'] = !empty($user_edit['data']) ? array_merge($backdrop_account->data, $user_edit['data']) : $backdrop_account->data;
    }
    foreach ($user_edit as $key => $value) {
      $backdrop_account->{$key} = $value;
    }
    if (!$updated_account = user_save($backdrop_account)) {
      watchdog('ldap_authentication', 'Failed to make changes to user %username updated %changed.', $watchdog_tokens, WATCHDOG_ERROR);
    }
    elseif ($auth_conf->emailUpdate == LDAP_AUTHENTICATION_EMAIL_UPDATE_ON_LDAP_CHANGE_ENABLE_NOTIFY) {
      if (isset($user_edit['mail'])) {
        $watchdog_tokens['%mail'] = $user_edit['mail'];
        backdrop_set_message(t('Your e-mail has been updated to match your current account (%mail).', $watchdog_tokens), 'status');
      }
      if (isset($user_edit['name'])) {
        $watchdog_tokens['%new_username'] = $user_edit['name'];
        backdrop_set_message(t('Your old account username %username has been updated to %new_username.', $watchdog_tokens), 'status');
      }
    }
  }

  /**
   * VI.C: no existing Backdrop account. consider provisioning Backdrop account.
   */
  if (!$backdrop_account_exists) {

    // VI.C.1 Do not provision Backdrop account if another account has same email.
    if (($auth_conf->ldapUser->acctCreation == LDAP_USER_ACCOUNTS_WITH_SAME_EMAIL_DISABLED) && ($account_with_same_email = user_load_by_mail($ldap_user['mail']))) {
      $error = TRUE;
      /**
       * username does not exist but email does.
       * Since user_external_login_register does not deal with mail attribute
       * and the email conflict error needs to be caught beforehand, need to
       * throw error here.
       */
      if ($auth_conf->templateUsageResolveConflict && (!$email_template_used)) {
        if ($detailed_watchdog_log) {
          watchdog('ldap_authentication', 'Conflict detected, using template generated email for %username', $watchdog_tokens, WATCHDOG_DEBUG);
        }
        _ldap_authentication_replace_user_email($ldap_user, $auth_conf, $email_template_tokens);
        $email_template_used = TRUE;
        // Recheck with the template email to make sure it doesn't also exist.
        if ($account_with_same_email = user_load_by_mail($ldap_user['mail'])) {
          $error = TRUE;
        }
      }
      if ($error) {
        $watchdog_tokens['%duplicate_name'] = $account_with_same_email->name;
        watchdog('ldap_authentication', 'LDAP user with DN %dn has email address
          (%mail) conflict with a backdrop user %duplicate_name', $watchdog_tokens, WATCHDOG_ERROR);
        backdrop_set_message(t('Another user already exists in the system with the same email address. You should contact the system administrator in order to solve this conflict.'), 'error');
        return;
      }
    }

    // VI.C.2 Do not provision Backdrop account if provisioning disabled.
    if (!$auth_conf->ldapUser->provisionEnabled(LDAP_USER_PROV_DIRECTION_TO_BACKDROP_USER, LDAP_USER_BACKDROP_USER_PROV_ON_AUTHENTICATE)) {
      watchdog('ldap_user', 'Backdrop account for authname=%authname account name=%account_name_attr does not exist and provisioning of Backdrop accounts on authentication is not enabled', $watchdog_tokens, WATCHDOG_INFO);
      return;
    }

    /**
     * VI.C.3 Provision Backdrop account.
     * New ldap_authentication provisioned account could
     * letuser_external_login_register create the account and set authmaps, but
     * would need to add mail and any other user->data data in
     * hook_user_presave which would mean requerying ldap or having a global
     * variable. At this point the account does not exist, so there is no
     * reason not to create it here.
     *
     * @todo create patch for core user module's user_external_login_register
     * to deal with new external accounts a little tweak to add user->data and
     * mail etc as parameters would make it more useful for external
     * authentication modules.
     */
    $user_register = config_get('system.core', 'user_register');
    if ($user_register === NULL) {
      $user_register = USER_REGISTER_VISITORS_ADMINISTRATIVE_APPROVAL;
    }
    if ($auth_conf->ldapUser->acctCreation == LDAP_USER_ACCT_CREATION_USER_SETTINGS_FOR_LDAP &&
            $user_register == USER_REGISTER_ADMINISTRATORS_ONLY) {
      watchdog('ldap_user', 'Failed to create account for %backdrop_accountname. Administrative user must create user.',
          $watchdog_tokens, WATCHDOG_ERROR);
      form_set_error('name', t('Server Error: Attempt to create account for %backdrop_accountname failed. Administrative user must create user.',
          $watchdog_tokens));
      return;
    }
    $user_register = config_get('system.core', 'user_register');
    if ($user_register === NULL) {
      $user_register = USER_REGISTER_VISITORS_ADMINISTRATIVE_APPROVAL;
    }
    if ($auth_conf->ldapUser->acctCreation == LDAP_AUTHENTICATION_ACCT_CREATION_USER_SETTINGS_FOR_LDAP &&
        $user_register == USER_REGISTER_VISITORS_ADMINISTRATIVE_APPROVAL) {
      // If admin approval required, set status to 0.
      $user_edit = ['name' => $backdrop_accountname, 'status' => 0];
    }
    else {
      $user_edit = ['name' => $backdrop_accountname, 'status' => 1];
    }

    // If the email template was used, we want to pass in the email that was
    // generated so that its not overridden by the provisioner.
    if ($email_template_used) {
      $user_edit['mail'] = $ldap_user['mail'];
    }

    // don't pass in ldap user to provisionBackdropAccount, because want to requery with correct attributes needed
    // this may be a case where efficiency dictates querying for all attributes.
    $backdrop_account = $auth_conf->ldapUser->provisionBackdropAccount(NULL, $user_edit, $ldap_user, TRUE);

    if ($backdrop_account === FALSE) {
      watchdog('ldap_user', 'Failed to find or create %backdrop_accountname on logon.', $watchdog_tokens, WATCHDOG_ERROR);
      form_set_error('name', t('Server Error: Failed to create Backdrop user account for %backdrop_accountname', $watchdog_tokens));
      return;
    }
    else {
      ldap_user_user_set_authmaps($backdrop_account, ['authname_ldap_user' => $authname]);
      // Using Rules allows emails to be fired and many other possible reactions
      // to the creation of a user.
      if (module_exists('rules')) {
        rules_invoke_event('ldap_user_created', $backdrop_account, $email_template_used);
      }
    }
  }

  /**
  * we now have valid, ldap authenticated username with an account authmapped to ldap_authentication.
  * since user_external_login_register can't deal with user mail attribute and doesn't do much else, it is not
  * being used here.
  *
  * without doing the user_login_submit,
  * [#1009990],[#1865938]
  */

  $form_state['uid'] = $backdrop_account->uid;
  return ($return_user) ? $backdrop_account : NULL;
}

/**
 * Given authname, determine if corresponding backdrop account exists and is authmapped.
 */
function ldap_authentication_corresponding_backdrop_user($authname, $auth_conf, &$watchdog_tokens) {
  $detailed_watchdog_log = (module_exists('ldap_help')) ? config_get('ldap_help.settings', 'ldap_help_watchdog_detail') : 0;
  if (!($backdrop_account = user_load_by_name($authname)) && !($backdrop_account = user_load_by_mail($authname))) {
    $uid = db_query("SELECT uid FROM {ldap_authmap} WHERE authname = :authname AND module = 'ldap_user'", [':authname' => $authname])->fetchColumn();
    $backdrop_account = $uid ? user_load($uid) : FALSE;
  }

  if (is_object($backdrop_account)) {
    // Patch 1599632.
    $authmaps = ldap_user_user_get_authmaps($authname);
    $backdrop_account_is_authmapped = isset($authmaps['ldap_user']);
    $user_data = $backdrop_account->data;
    if ($backdrop_account->uid == 1 && $detailed_watchdog_log) {
      watchdog('ldap_authentication', '%username : Backdrop username maps to user 1, so do not authenticate with ldap', $watchdog_tokens, WATCHDOG_DEBUG);
    }
    elseif ($detailed_watchdog_log) {
      watchdog('ldap_authentication', '%username : Backdrop User Account found. Continuing on to attempt ldap authentication', $watchdog_tokens, WATCHDOG_DEBUG);
    }
  }
  // Account does not exist.
  else {
    $backdrop_account_is_authmapped = FALSE;
    if ($auth_conf->ldapUser->createLDAPAccounts == FALSE) {
      if ($detailed_watchdog_log) {
        watchdog('ldap_authentication', '%username : Backdrop User Account not found and configuration is set to not create new accounts.', $watchdog_tokens, WATCHDOG_DEBUG);
      }
    }
    if ($detailed_watchdog_log) {
      watchdog('ldap_authentication', '%username : Existing Backdrop User Account not found. Continuing on to attempt ldap authentication', $watchdog_tokens, WATCHDOG_DEBUG);
    }
  }
  return [$backdrop_account, $backdrop_account_is_authmapped];
}

/**
 *
 */
function ldap_authentication_test_credentials($auth_conf, $sso_login, $authname, $password, &$watchdog_tokens) {
  $detailed_watchdog_log = (module_exists('ldap_help')) ? config_get('ldap_help.settings', 'ldap_help_watchdog_detail') : 0;
  $authentication_result = LDAP_AUTHENTICATION_RESULT_FAIL_GENERIC;
  $ldap_user = FALSE;
  $ldap_server = NULL;
  foreach ($auth_conf->enabledAuthenticationServers as $sid => $ldap_server) {
    $watchdog_tokens['%sid'] = $sid;
    $watchdog_tokens['%bind_method'] = $ldap_server->bind_method;
    if ($detailed_watchdog_log) {
      watchdog('ldap_authentication', '%username : Trying server %sid where bind_method = %bind_method', $watchdog_tokens, WATCHDOG_DEBUG);
    }

    // #1 CONNECT TO SERVER.
    $authentication_result = LDAP_AUTHENTICATION_RESULT_FAIL_GENERIC;
    $result = $ldap_server->connect();
    if ($result != LDAP_SUCCESS) {
      $authentication_result = LDAP_AUTHENTICATION_RESULT_FAIL_CONNECT;
      $watchdog_tokens['%err_msg'] = $ldap_server->errorMsg('ldap');
      if ($detailed_watchdog_log) {
        watchdog('ldap_authentication', '%username : Failed connecting to %sid. Error: %err_msg', $watchdog_tokens, WATCHDOG_DEBUG);
      }
      $watchdog_tokens['%err_msg'] = NULL;
      // Next server, please.
      continue;
    }
    elseif ($detailed_watchdog_log) {
      watchdog('ldap_authentication', '%username : Success at connecting to %sid', $watchdog_tokens, WATCHDOG_DEBUG);
    }

    $bind_success = FALSE;
    if ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT) {
      $bind_success = ($ldap_server->bind(NULL, NULL, FALSE) == LDAP_SUCCESS);
    }
    elseif ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_ANON ||
        $ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_ANON_USER) {
      $bind_success = ($ldap_server->bind(NULL, NULL, TRUE) == LDAP_SUCCESS);
    }
    elseif ($sso_login) {
      watchdog('ldap_authentication', 'Trying to use SSO with LDAP_SERVERS_BIND_METHOD_USER bind method.', $watchdog_tokens, WATCHDOG_ERROR);
    }
    elseif ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_USER && $sso_login == FALSE) {
      // With sso enabled this method of binding isn't valid.
      foreach ($ldap_server->basedn as $basedn) {
        $search = ['%basedn', '%username'];
        $transformname = $ldap_server->userUsernameToLdapNameTransform($authname, $watchdog_tokens);
        $replace = [$basedn, $transformname];
        $userdn = str_replace($search, $replace, $ldap_server->user_dn_expression);
        $bind_success = ($ldap_server->bind($userdn, $password, FALSE) == LDAP_SUCCESS);
        if ($bind_success) {
          break;
        }
      }
    }
    else {
      watchdog('ldap_authentication', 'No bind method set in ldap_server->bind_method in _ldap_authentication_user_login_authenticate_validate.', $watchdog_tokens, WATCHDOG_ERROR);
    }

    if (!$bind_success) {
      if ($detailed_watchdog_log) {
        $watchdog_tokens['%err_text'] = $ldap_server->errorMsg('ldap');
        watchdog('ldap_authentication', '%username : Trying server %sid where bind_method = %bind_method. Error: %err_text', $watchdog_tokens, WATCHDOG_DEBUG);
        $watchdog_tokens['%err_text'] = NULL;
      }
      $authentication_result = ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_USER) ? LDAP_AUTHENTICATION_RESULT_FAIL_CREDENTIALS : LDAP_AUTHENTICATION_RESULT_FAIL_BIND;
      // If bind fails, onto next server.
      continue;
    }

    // #3 DOES USER EXIST IN SERVER'S LDAP.
    if ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_ANON_USER) {
      $ldap_user = $ldap_server->userUserNameToExistingLdapEntry($authname);
    }
    elseif ($sso_login) {
      $ldap_user = $ldap_server->userUserNameToExistingLdapEntry($authname);
      if ($detailed_watchdog_log) {
        $watchdog_tokens['%result'] = var_export($result, TRUE);
        watchdog('ldap_authentication', '%username : attempting single sign-on
          login in bind_method of LDAP_SERVERS_BIND_METHOD_USER. Result of
          userUserNameToExistingLdapEntry: <pre>%result</pre>', $watchdog_tokens, WATCHDOG_DEBUG);
      }
    }
    else {
      $ldap_user = $ldap_server->userUserNameToExistingLdapEntry($authname);
    }

    if (!$ldap_user) {
      if ($detailed_watchdog_log) {
        $watchdog_tokens['%err_text'] = $ldap_server->errorMsg('ldap');
        watchdog('ldap_authentication', '%username : Trying server %sid where bind_method = %bind_method. Error: %err_text', $watchdog_tokens, WATCHDOG_DEBUG);
        $watchdog_tokens['%err_text'] = NULL;
      }
      if ($ldap_server->ldapErrorNumber()) {
        $authentication_result = LDAP_AUTHENTICATION_RESULT_FAIL_SERVER;
      }
      else {
        $authentication_result = LDAP_AUTHENTICATION_RESULT_FAIL_FIND;
      }
      // Next server, please.
      continue;
    }

    $watchdog_tokens['%dn'] = $ldap_user['dn'];
    $watchdog_tokens['%mail'] = $ldap_user['mail'];

    /**
     * #4 CHECK ALLOWED AND EXCLUDED LIST AND PHP FOR ALLOWED USERS
     */
    if (!$auth_conf->allowUser($authname, $ldap_user)) {
      $authentication_result = LDAP_AUTHENTICATION_RESULT_FAIL_DISALLOWED;
      // Regardless of how many servers, disallowed user fails.
      break;
    }

    /**
     * #5 TEST PASSWORD
     */
    $credentials_pass = FALSE;
    if ($sso_login) {
      /** If we have $sso_login passed in as true from the fake form state in
        * passed from _ldap_authentication_user_login_sso(), we will be relying
        * on the webserver for actually authenticating the user, either by NTLM
        * or user/password if configured as a fallback. Since the webserver has
        * already authenticated the user, and the web server only contains the
        * user's LDAP user name, instead of binding on the username/pass, we
        * simply look up the user's account in LDAP, and make sure it matches
        * what is contained in the global $_SERVER array populated by the web
        * server authentication.
        */
      $credentials_pass = (boolean) ($ldap_user);
    }
    elseif ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_USER) {
      /**
        * With user bind method, the only way we can reach this part of the
        * code is when the pw has already been checked and $ldap_user could be
        * loaded, so we're good to go.
        */
      $credentials_pass = TRUE;
    }
    else {
      $credentials_pass = ($ldap_server->bind($ldap_user['dn'], $password, FALSE) == LDAP_SUCCESS);
    }
    if (!$credentials_pass) {
      if ($detailed_watchdog_log) {
        $watchdog_tokens['%err_text'] = $ldap_server->errorMsg('ldap');
        watchdog('ldap_authentication', '%username : Testing user credentials on server %sid where bind_method = %bind_method. Error: %err_text', $watchdog_tokens, WATCHDOG_DEBUG);
        $watchdog_tokens['%err_text'] = NULL;
      }
      $authentication_result = LDAP_AUTHENTICATION_RESULT_FAIL_CREDENTIALS;
      // Next server, please.
      continue;
    }
    else {
      $authentication_result = LDAP_AUTHENTICATION_RESULT_SUCCESS;
      if ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_ANON_USER) {
        // After successful bind, lookup user again to get private attributes.
        $ldap_user = $ldap_server->userUserNameToExistingLdapEntry($authname);
        $watchdog_tokens['%mail'] = $ldap_user['mail'];
      }
      if ($ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT ||
          $ldap_server->bind_method == LDAP_SERVERS_BIND_METHOD_ANON_USER) {
        $ldap_server->disconnect();
      }
      // Update ldapUser with the sid of the server that the user authenticated
      // on if that option was enabled in the LDAP user configuration.
      if ($auth_conf->ldapUser->backdropAcctProvisionServer == LDAP_USER_AUTH_SERVER_SID) {
        $auth_conf->ldapUser->backdropAcctProvisionServer = $ldap_server->sid;
      }
      // Success.
      break;
    }
  }
  // End loop through servers.
  $watchdog_tokens['%result'] = $result;
  $watchdog_tokens['%auth_result'] = $authentication_result;
  $watchdog_tokens['%err_text'] = _ldap_authentication_err_text($authentication_result);
  if ($detailed_watchdog_log) {
    watchdog('ldap_authentication', '%username : Authentication result id=%result auth_result=%auth_result (%err_text)', $watchdog_tokens, WATCHDOG_DEBUG);
  }

  return [$authentication_result, $ldap_user, $ldap_server];
}

/**
 *
 */
function ldap_authentication_fail_response($authentication_result, $auth_conf, $detailed_watchdog_log, &$watchdog_tokens) {
  $watchdog_tokens['%err_text'] = _ldap_authentication_err_text($authentication_result);
  // Fail scenario 1. ldap auth exclusive and failed  throw error so no other authentication methods are allowed.
  if ($auth_conf->authenticationMode == LDAP_AUTHENTICATION_EXCLUSIVE) {
    if ($detailed_watchdog_log) {
      watchdog('ldap_authentication', '%username : setting error because failed at ldap and
        LDAP_AUTHENTICATION_EXCLUSIVE is set to true. So need to stop authentication of Backdrop user that is not user 1.
        error message: %err_text', $watchdog_tokens, WATCHDOG_DEBUG);
    }
    form_set_error('name', $watchdog_tokens['%err_text']);
  }
  else {
    // Fail scenario 2. simply fails ldap. return false, but don't throw form error
    // don't show user message, may be using other authentication after this that may succeed.
    if ($detailed_watchdog_log) {
      watchdog('ldap_authentication',
        '%username : Failed ldap authentication.
        User may have authenticated successfully by other means in a mixed authentication site.
        LDAP Authentication Error #: %auth_result  error message: %err_text',
        $watchdog_tokens,
        WATCHDOG_DEBUG
        );
    }
  }
}

/**
 * Get human readable authentication error string.
 *
 * @param int $error
 *   as LDAP_AUTHENTICATION_RESULT_* constant defined in
 *   ldap_authentication.module.
 *
 * @return string human readable error text
 */
function _ldap_authentication_err_text($error) {

  $msg = t('unknown error: ' . $error);
  switch ($error) {
    case LDAP_AUTHENTICATION_RESULT_FAIL_CONNECT:
      $msg = t('Failed to connect to ldap server');
      break;

    case LDAP_AUTHENTICATION_RESULT_FAIL_BIND:
      $msg = t('Failed to bind to ldap server');
      break;

    case LDAP_AUTHENTICATION_RESULT_FAIL_FIND:
      $msg = t('Sorry, unrecognized username or password.');
      break;

    case LDAP_AUTHENTICATION_RESULT_FAIL_DISALLOWED:
      $msg = t('User disallowed');
      break;

    case LDAP_AUTHENTICATION_RESULT_FAIL_CREDENTIALS:
      $msg = t('Sorry, unrecognized username or password.');
      break;

    case LDAP_AUTHENTICATION_RESULT_FAIL_GENERIC:
      $msg = t('Sorry, unrecognized username or password.');
      break;

    case LDAP_AUTHENTICATION_RESULT_FAIL_SERVER:
      $msg = t('Authentication Server or Configuration Error.');
      break;

  }

  return $msg;
}

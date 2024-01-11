<?php

/**
 * @file
 * Class to represent configuration of ldap authorizations to backdrop roles.
 */

module_load_include('php', 'ldap_authorization', 'LdapAuthorizationConsumerAbstract.class');
/**
 *
 */
class LdapAuthorizationConsumerBackdropRole extends LdapAuthorizationConsumerAbstract {

  public $consumerType = 'backdrop_role';
  public $allowConsumerObjectCreation = TRUE;

  public $defaultConsumerConfProperties = [
    'onlyApplyToLdapAuthenticated' => TRUE,
    'useMappingsAsFilter' => TRUE,
    'synchOnLogon' => TRUE,
    'revokeLdapProvisioned' => TRUE,
    'regrantLdapProvisioned' => TRUE,
    'createConsumers' => TRUE,
  ];

  /**
   *
   */
  public function __construct($consumer_type = NULL) {
    $params = ldap_authorization_backdrop_role_ldap_authorization_consumer();
    parent::__construct('backdrop_role', $params['backdrop_role']);
  }

  /**
   * @see LdapAuthorizationConsumerAbstract::createConsumer
   */
  public function createConsumer($consumer_id, $consumer) {
    $roles_by_consumer_id = $this->existingRolesByRoleName();
    $existing_role = isset($roles_by_consumer_id[$consumer_id]) ? $roles_by_consumer_id[$consumer_id] : FALSE;

    if ($existing_role) {
      // Role exists.
      return FALSE;
    }
    elseif (backdrop_strlen($consumer_id) > 63) {
      watchdog('ldap_authorization_backdrop_role', 'Tried to create backdrop role
        with name of over 63 characters (%group_name). Please correct your
        backdrop ldap_authorization settings', ['%group_name' => $consumer_id]);
      return FALSE;
    }

    $new_role = new stdClass();
    $new_role->name = empty($consumer['value']) ? $consumer_id : $consumer['value'];
    $new_role->label = $new_role->name;
    if (!($status = user_role_save($new_role))) {
      // If role is not created, remove from array to user object doesn't have it stored as granted.
      watchdog('user', 'failed to create backdrop role %role in ldap_authorizations module', ['%role' => $new_role->name]);
      return FALSE;
    }
    else {
      // Flush existingRolesByRoleName cache after creating new role.
      $roles_by_consumer_id = $this->existingRolesByRoleName(TRUE);
      watchdog('user', 'created backdrop role %role in ldap_authorizations module', ['%role' => $new_role->name]);
    }
    return TRUE;
  }

  /**
   * @see LdapAuthorizationConsumerAbstract::populateConsumersFromConsumerIds
   */
  public function populateConsumersFromConsumerIds(&$consumers, $create_missing_consumers = FALSE) {

    $roles_by_consumer_id = $this->existingRolesByRoleName(TRUE);
    foreach ($consumers as $consumer_id => $consumer) {

      // Role marked as not existing.
      if (!$consumer['exists']) {
        // Check if is existing.
        if (isset($roles_by_consumer_id[$consumer_id])) {
          $consumer['exists'] = TRUE;
          $consumer['value'] = $roles_by_consumer_id[$consumer_id]['role_name'];
          $consumer['name'] = $consumer['map_to_string'];
          $consumer['map_to_string'] = $roles_by_consumer_id[$consumer_id]['role_name'];
        }
        elseif ($create_missing_consumers) {
          $consumer['value'] = $consumer['map_to_string'];
          $consumer['name'] = $consumer['map_to_string'];
          $result = $this->createConsumer($consumer_id, $consumer);
          $consumer['exists'] = $result;
        }
        else {
          $consumer['exists'] = FALSE;
        }
      }
      elseif (empty($consumer['value'])) {
        $consumer['value'] = $roles_by_consumer_id[$consumer_id]['role_name'];
      }
      $consumers[$consumer_id] = $consumer;
    }
  }

  /**
   *
   */
  public function revokeSingleAuthorization(&$user, $consumer_id, $consumer, &$user_auth_data, $user_save = FALSE, $reset = FALSE) {

    $role_name_lcase = $consumer_id;
    $role_name = empty($consumer['value']) ? $consumer_id : $consumer['value'];
    $rid = $this->getBackdropRoleIdFromRoleName($role_name);
    if (!$rid) {
      // Role id not found.
      $result = FALSE;
    }
    // User doesn't have role.
    elseif (!in_array($rid, $user->roles)) {
      if (isset($user_auth_data[$consumer_id])) {
        unset($user_auth_data[$consumer_id]);
      }
      $result = TRUE;
    }
    else {
      $user->roles = array_diff($user->roles, [$rid]);
      $user_edit = ['roles' => $user->roles];
      $account = user_load($user->uid);
      foreach ($user_edit as $key => $value) {
        $account->{$key} = $value;
      }
      $account->save();
      $user = $account;
      $result = ($user && !in_array($rid, $user->roles));
      if ($result && isset($user_auth_data[$consumer_id])) {
        unset($user_auth_data[$consumer_id]);
      }
    }

    if ($this->detailedWatchdogLog) {
      watchdog('ldap_authorization', 'LdapAuthorizationConsumerBackdropRole.revokeSingleAuthorization()
        revoked:  rid=%rid, role_name=%role_name for username=%username, result=%result',
        [
          '%rid' => $rid,
          '%role_name' => $role_name,
          '%username' => $user->name,
          '%result' => $result,
        ], WATCHDOG_DEBUG);
    }

    return $result;

  }

  /**
   * Extends grantSingleAuthorization()
   */
  public function grantSingleAuthorization(&$user, $consumer_id, $consumer, &$user_auth_data, $user_save = FALSE, $reset = FALSE) {

    $role_name_lcase = $consumer_id;
    $role_name = empty($consumer['value']) ? $consumer_id : $consumer['value'];
    $rid = $this->getBackdropRoleIdFromRoleName($role_name);
    if (is_null($rid)) {
      watchdog('ldap_authorization', 'LdapAuthorizationConsumerBackdropRole.grantSingleAuthorization()
      failed to grant %username the role %role_name because role does not exist',
      ['%role_name' => $role_name, '%username' => $user->name],
      WATCHDOG_ERROR);
      return FALSE;
    }

    $user->roles[] = $rid;
    $user_edit = ['roles' => $user->roles];
    if ($this->detailedWatchdogLog) {
      watchdog('ldap_authorization', 'grantSingleAuthorization in backdrop role' . print_r($user, TRUE), [], WATCHDOG_DEBUG);
    }

    $account = user_load($user->uid);
    foreach ($user_edit as $key => $value) {
      $account->{$key} = $value;
    }
    $account->save();
    $user = $account;
    $result = ($user && in_array($rid, $user->roles));

    if ($this->detailedWatchdogLog) {
      watchdog('ldap_authorization', 'LdapAuthorizationConsumerBackdropRole.grantSingleAuthorization()
        granted: rid=%rid, role_name=%role_name for username=%username, result=%result',
        [
          '%rid' => $rid,
          '%role_name' => $role_name,
          '%username' => $user->name,
          '%result' => $result,
        ], WATCHDOG_DEBUG);
    }

    return $result;

  }

  /**
   *
   */
  public function usersAuthorizations(&$user) {
    $authorizations = [];
    $user_roles = user_roles();
    foreach ($user->roles as $rid) {
      $authorizations[] = backdrop_strtolower($user_roles[$rid]);
    }
    return $authorizations;
  }

  /**
   *
   */
  public function validateAuthorizationMappingTarget($mapping, $form_values = NULL, $clear_cache = FALSE) {

    $has_form_values = is_array($form_values);
    $message_type = NULL;
    $message_text = NULL;
    $role_name = $mapping['normalized'];
    $tokens = ['!map_to' => $role_name];
    $roles_by_name = $this->existingRolesByRoleName();
    $pass = isset($roles_by_name[backdrop_strtolower($role_name)]);

    if (!$pass) {
      $message_text = '"' . t('Backdrop role') . ' ' . t('!map_to', $tokens) . '" ' . t('does not map to any existing Backdrop roles.');
      if ($has_form_values) {
        $create_consumers = (isset($form_values['synchronization_actions']['create_consumers']) && $form_values['synchronization_actions']['create_consumers']);
      }
      else {
        $create_consumers = $this->consumerConf->createConsumers;
      }
      if ($create_consumers && $this->allowConsumerObjectCreation) {
        $message_type = 'warning';
        $message_text .= ' ' . t('"!map_to" will be created when needed. If "!map_to" is not intentional, please fix it.', $tokens);
      }
      elseif (!$this->allowConsumerObjectCreation) {
        $message_type = 'error';
        $message_text .= ' ' . t('Since automatic Backdrop role creation is not possible with this module, an existing role must be mapped to.');
      }
      elseif (!$create_consumers) {
        $message_type = 'error';
        $message_text .= ' ' . t('Since automatic Backdrop role creation is disabled, an existing role must be mapped to. Either enable role creation or map to an existing role.');
      }
    }
    return [$message_type, $message_text];
  }

  /**
   * @param string mixed case $role_name
   * @return integer role id
   */
  private function getBackdropRoleIdFromRoleName($role_name) {
    $role_ids_by_name = $this->existingRolesByRoleName();
    $role_name_lowercase = backdrop_strtolower($role_name);
    return empty($role_ids_by_name[$role_name_lowercase]) ? NULL : $role_ids_by_name[$role_name_lowercase]['rid'];
  }

  /**
   * @param bool $reset
   *   to reset static values.
   * @return associative array() keyed on lowercase role names with values
   *   of array('rid' => role id, 'role_name' => mixed case role name)
   */
  public function existingRolesByRoleName($reset = FALSE) {

    static $roles_by_name;

    if ($reset || !is_array($roles_by_name)) {
      $roles_by_name = [];
      foreach (array_flip(user_roles(TRUE)) as $role_name => $rid) {
        $roles_by_name[backdrop_strtolower($role_name)]['rid'] = $rid;
        $roles_by_name[backdrop_strtolower($role_name)]['role_name'] = $role_name;
      }
    }
    return $roles_by_name;
  }

  /**
   * @see LdapAuthorizationConsumerAbstract::normalizeMappings
   */
  public function normalizeMappings($mappings) {

    $new_mappings = [];
    // In rid => role name format.
    $roles = user_roles(TRUE);
    $roles_by_name = array_flip($roles);
    foreach ($mappings as $i => $mapping) {
      $new_mapping = [];
      $new_mapping['user_entered'] = $mapping[1];
      $new_mapping['from'] = $mapping[0];
      $new_mapping['normalized'] = $mapping[1];
      $new_mapping['simplified'] = $mapping[1];
      $create_consumers = (boolean) ($this->allowConsumerObjectCreation && $this->consumerConf->createConsumers);
      $new_mapping['valid'] = (boolean) (!$create_consumers && !empty($roles_by_name[$mapping[1]]));
      $new_mapping['error_message'] = ($new_mapping['valid']) ? '' : t("Role %role_name does not exist and role creation is not enabled.", ['%role' => $mapping[1]]);
      $new_mappings[] = $new_mapping;
    }

    return $new_mappings;
  }

  /**
   * @see ldapAuthorizationConsumerAbstract::convertToFriendlyAuthorizationIds
   */
  public function convertToFriendlyAuthorizationIds($authorizations) {
    $authorization_ids_friendly = [];
    foreach ($authorizations as $authorization_id => $authorization) {
      $authorization_ids_friendly[] = isset($authorization['name']) ? $authorization['name'] . '  (' . $authorization_id . ')' : $authorization_id;
    }
    return $authorization_ids_friendly;
  }

}

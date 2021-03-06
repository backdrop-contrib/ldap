<?php

/**
 * @file
 * LDAP Server Admin Class.
 */

module_load_include('php', 'ldap_servers', 'LdapServer.class');
/**
 *
 */
class LdapServerAdmin extends LdapServer {

  public $bindpw_new = FALSE;
  public $bindpw_clear = FALSE;

  /**
   * @param $type
   *   = 'all', 'enabled'
   */
  public static function getLdapServerObjects($sid = NULL, $type = NULL, $class = 'LdapServer', $reset = FALSE) {
    $servers = [];
    $select = [];
    try {
      $configs = config_get_names_with_prefix('ldap.server.');
      foreach ($configs as $config) {
        $select[] = (object) config_get($config, 'config');
      }
    }
    catch (Exception $e) {
      backdrop_set_message(t('server index query failed. Message = %message, query= %query',
        ['%message' => $e->getMessage(), '%query' => $e->query_string]), 'error');
      return [];
    }
    foreach ($select as $result) {
      $servers[$result->sid] = ($class == 'LdapServer') ? new LdapServer($result->sid) : new LdapServerAdmin($result->sid);
    }
    return $servers;

  }

  /**
   *
   */
  public function __construct($sid) {
    parent::__construct($sid);
  }

  /**
   *
   */
  protected function populateFromBackdropForm($op, $values) {
    $this->inDatabase = ($op == 'edit');
    $this->sid = trim($values['sid']);
    $this->name = trim($values['name']);
    $this->status = ($values['status']) ? 1 : 0;
    $this->ldap_type = trim($values['ldap_type']);
    $this->address = trim($values['address']);
    $this->port = trim($values['port']);
    $this->tls = trim($values['tls']);
    $this->followrefs = trim($values['followrefs']);
    $this->bind_method = trim($values['bind_method']);
    $this->binddn = trim($values['binddn']);
    if (trim($values['bindpw'])) {
      $this->bindpw_new = trim($values['bindpw']);
    }
    $this->user_dn_expression = trim($values['user_dn_expression']);
    $this->basedn = $this->linesToArray(trim($values['basedn']));
    $this->user_attr = backdrop_strtolower(trim($values['user_attr']));
    $this->picture_attr = backdrop_strtolower(trim($values['picture_attr']));
    $this->account_name_attr = backdrop_strtolower(trim($values['account_name_attr']));
    $this->mail_attr = backdrop_strtolower(trim($values['mail_attr']));
    $this->mail_template = trim($values['mail_template']);
    $this->unique_persistent_attr = backdrop_strtolower(trim($values['unique_persistent_attr']));
    $this->unique_persistent_attr_binary = trim($values['unique_persistent_attr_binary']);
    $this->ldapToBackdropUserPhp = $values['ldap_to_backdrop_user'];
    $this->testingBackdropUsername = trim($values['testing_backdrop_username']);
    $this->testingBackdropUserDn = trim($values['testing_backdrop_user_dn']);
    $this->groupFunctionalityUnused = $values['grp_unused'];
    $this->groupObjectClass = backdrop_strtolower(trim($values['grp_object_cat']));
    $this->groupNested = trim($values['grp_nested']);

    $this->groupUserMembershipsAttrExists = trim($values['grp_user_memb_attr_exists']);
    $this->groupUserMembershipsAttr = backdrop_strtolower(trim($values['grp_user_memb_attr']));

    $this->groupMembershipsAttr = backdrop_strtolower(trim($values['grp_memb_attr']));

    $this->groupMembershipsAttrMatchingUserAttr = backdrop_strtolower(trim($values['grp_memb_attr_match_user_attr']));

    $this->groupDeriveFromDn = trim($values['grp_derive_from_dn']);
    $this->groupDeriveFromDnAttr = backdrop_strtolower(trim($values['grp_derive_from_dn_attr']));
    $this->groupTestGroupDn = trim($values['grp_test_grp_dn']);
    $this->groupTestGroupDnWriteable = trim($values['grp_test_grp_dn_writeable']);

    $this->searchPagination = ($values['search_pagination']) ? 1 : 0;
    $this->searchPageSize = trim($values['search_page_size']);

  }

  /**
   * @param string enum $op
   *   'add', 'update'.
   */
  public function save($op) {

    $values = new stdClass();

    foreach ($this->field_to_properties_map() as $field_name => $property_name) {
      $field_name_lcase = backdrop_strtolower($field_name);
      $values->{$field_name_lcase} = $this->{$property_name};
    }
    if (isset($this->bindpw) && $this->bindpw) {
      $values->bindpw = ldap_servers_encrypt($this->bindpw);
    }
    if ($this->bindpw_new) {
      $values->bindpw = ldap_servers_encrypt($this->bindpw_new);
    }
    elseif ($this->bindpw_clear) {
      $values->bindpw = NULL;
    }

    $values->tls = (int) $this->tls;
    $values->followrefs = (int) $this->followrefs;

    unset($values->numeric_sid);
    $config = config('ldap.server.' . $values->sid);
    $config->set('id', $values->sid);
    $config->set('name', $values->name);
    $config->set('config', $values);
    $config->save();
    ldap_servers_cache_clear();
    $this->inDatabase = TRUE;
  }

  /**
   *
   */
  public function delete($sid) {
    if ($sid == $this->sid) {
      $config = config('ldap.server.' . $sid);
      $result = $config->delete();
      $result = !empty($result);
      $this->inDatabase = FALSE;
      return $result;
    }
    else {
      return FALSE;
    }
  }

  /**
   *
   */
  public function getLdapServerActions() {
    $switch = ($this->status) ? 'disable' : 'enable';
    $actions = [];
    $actions[] = l(t('edit'), LDAP_SERVERS_MENU_BASE_PATH . '/servers/edit/' . $this->sid);
    if (property_exists($this, 'type')) {
      if ($this->type == 'Overridden') {
        $actions[] = l(t('revert'), LDAP_SERVERS_MENU_BASE_PATH . '/servers/delete/' . $this->sid);
      }
      if ($this->type == 'Normal') {
        $actions[] = l(t('delete'), LDAP_SERVERS_MENU_BASE_PATH . '/servers/delete/' . $this->sid);
      }
    }
    else {
      $actions[] = l(t('delete'), LDAP_SERVERS_MENU_BASE_PATH . '/servers/delete/' . $this->sid);
    }
    $actions[] = l(t('test'), LDAP_SERVERS_MENU_BASE_PATH . '/servers/test/' . $this->sid);
    $actions[] = l($switch, LDAP_SERVERS_MENU_BASE_PATH . '/servers/' . $switch . '/' . $this->sid);
    return $actions;
  }

  /**
   *
   */
  public function backdropForm($op) {

    $form['server'] = [
      '#type' => 'fieldset',
      '#title' => t('Connection settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    ];

    $form['bind_method'] = [
      '#type' => 'fieldset',
      '#title' => t('Binding Method'),
      '#description' => t('How the Backdrop system is authenticated by the LDAP server.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    ];

    $form['users'] = [
      '#type' => 'fieldset',
      '#title' => t('LDAP User to Backdrop User Relationship'),
      '#description' => t('How are LDAP user entries found based on Backdrop username or email? And vice-versa?
       Needed for LDAP Authentication and Authorization functionality.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    ];

    $form['groups'] = [
      '#type' => 'fieldset',
      '#title' => t('LDAP Group Configuration'),
      '#description' => t('How are groups defined on your LDAP server?  This varies slightly from one LDAP implementation to another
      such as Active Directory, Novell, OpenLDAP, etc. Check everything that is true and enter all the values you know.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    ];

    $supports = (ldap_servers_php_supports_pagination()) ? t('support pagination!') : t('NOT support pagination.');
    $form['pagination'] = [
      '#type' => 'fieldset',
      '#title' => t('LDAP Pagination'),
      '#description' => t('In PHP 5.4, pagination is supported in ldap queries.
      A patch to earlier versions of PHP also supports this.')
      . ' <strong>' . t('This PHP installation appears to') . ' ' . $supports . '</strong> '
      . '<p>' . t('The advantage to pagination support is that if an ldap server is setup to return only
      1000 entries at a time,
      you can use page through 1000 records at a time;
      without pagination you would never see more than the first 1000 entries.
      Pagination is most useful when large queries for batch creating or
      synching accounts are used. If you are not using this server for such
      tasks, its recommended to leave pagination disabled.') . '</p>',
      '#collapsible' => TRUE,
      '#collapsed' => !ldap_servers_php_supports_pagination(),
    ];

    $field_to_prop_maps = $this->field_to_properties_map();
    foreach ($this->fields() as $field_id => $field) {
      if (isset($field['form'])) {

        if (!isset($field['form']['required']) && isset($field['schema']['not null']) && $field['form']['#type'] != 'checkbox') {
          $field['form']['#required'] = (boolean) $field['schema']['not null'];
        }
        if (isset($field['schema']['length']) && !isset($field['form']['#maxlength'])) {
          $field['form']['#maxlength'] = $field['schema']['length'];
        }
        if (isset($field_to_prop_maps[$field_id])) {
          $field['form']['#default_value'] = $this->{$field_to_prop_maps[$field_id]};
        }
        $fieldset = @$field['form']['fieldset'];
        if ($fieldset) {
          unset($field['form']['fieldset']);
          $form[$fieldset][$field_id] = $field['form'];
        }
        else {
          $form[$field_id] = $field['form'];
        }
      }
    }

    $form['server']['sid']['#disabled'] = ($op == 'edit');

    if (!function_exists('ldap_set_rebind_proc')) {
      $form['server']['followrefs']['#disabled'] = TRUE;
      $form['server']['followrefs']['#description'] = t('This functionality is disabled because the function ldap_set_rebind_proc can not be found on this server. Perhaps your version of php does not have this function. See php.net/manual/en/function.ldap-set-rebind-proc.php') . $form['server']['followrefs']['#description'];
    }

    $form['server']['tls']['#required'] = FALSE;
    $form['server']['followrefs']['#required'] = FALSE;
    $form['bind_method']['bind_method']['#default_value'] = ($this->bind_method) ? $this->bind_method : LDAP_SERVERS_BIND_METHOD_DEFAULT;
    $form['users']['basedn']['#default_value'] = $this->arrayToLines($this->basedn);

    if ($this->bindpw) {
      $pwd_directions = t('You currently have a password stored in the database.
      Leave password field empty to leave password unchanged. Enter a new password
      to replace the current password. Check the checkbox below to simply
      remove it from the database.');
      $pwd_class = 'ldap-pwd-present';
    }
    else {
      $pwd_directions = t('No password is currently stored in the database.
      If you are using a service account, enter one.');
      if ($this->bind_method == LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT) {
        $pwd_class = 'ldap-pwd-abscent';
      }
      else {
        $pwd_class = 'ldap-pwd-not-applicable';
      }
    }

    $action = ($op == 'add') ? 'Add' : 'Update';
    $form['submit'] = [
      '#type' => 'submit',
      '#value' => $action,
      '#weight' => 100,
    ];

    return $form;

  }

  /**
   *
   */
  public function backdropFormValidate($op, $values) {
    $errors = [];

    if ($op == 'delete') {
      if (!$this->sid) {
        $errors['server_id_missing'] = 'Server id missing from delete form.';
      }
      $warnings = module_invoke_all('ldap_server_in_use', $this->sid, $this->name);
      if (count($warnings)) {
        $errors['status'] = join("<br/>", array_values($warnings));
      }

    }
    else {
      $this->populateFromBackdropForm($op, $values);
      $errors = $this->validate($op);
    }
    return $errors;
  }

  /**
   *
   */
  protected function validate($op) {
    $errors = [];
    if ($op == 'add') {
      $ldap_servers = $this->getLdapServerObjects(NULL, 'all');
      if (count($ldap_servers)) {
        foreach ($ldap_servers as $sid => $ldap_server) {
          if ($this->name == $ldap_server->name) {
            $errors['name'] = t('An LDAP server configuration with the  name %name already exists.', ['%name' => $this->name]);
          }
          elseif ($this->sid == $ldap_server->sid) {
            $errors['sid'] = t('An LDAP server configuration with the  id %sid  already exists.', ['%sid' => $this->sid]);
          }
        }
      }
    }

    // Check that no modules use this server.
    if ($this->status == 0) {
      $warnings = module_invoke_all('ldap_server_in_use', $this->sid, $this->name);
      if (count($warnings)) {
        $errors['status'] = join("<br/>", array_values($warnings));
      }
    }

    if (!is_numeric($this->port)) {
      $errors['port'] = t('The TCP/IP port must be an integer.');
    }

    if ($this->bind_method == LDAP_SERVERS_BIND_METHOD_USER && !$this->user_dn_expression) {
      $errors['user_dn_expression'] = t('When using "Bind with Users Credentials", Expression for user DN is required');
    }

    if ($this->mail_attr && $this->mail_template) {
      $errors['mail_attr'] = t('Mail attribute or Mail Template may be used. Not both.');
    }

    if ($this->bind_method == LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT && !$this->binddn) {
      $errors['binddn'] = t('When using "Bind with Service Account", Bind DN is required.');
    }
    if ($op == 'add') {
      if ($this->bind_method == LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT &&
        (($op == 'add' && !$this->bindpw_new) || ($op != 'add' && !$this->bindpw))
      ) {
        $errors['bindpw'] = t('When using "Bind with Service Account", Bind password is required.');
      }
    }

    return $errors;
  }

  /**
   *
   */
  public function backdropFormWarnings($op, $values, $has_errors = NULL) {
    $errors = [];

    if ($op == 'delete') {
      if (!$this->sid) {
        $errors['server_id_missing'] = t('Server id missing from delete form.');
      }
    }
    else {
      $this->populateFromBackdropForm($op, $values);
      $warnings = $this->warnings($op, $has_errors);
    }
    return $warnings;
  }

  /**
   *
   */
  protected function warnings($op, $has_errors = NULL) {

    $warnings = [];
    if ($this->ldap_type) {
      $defaults = ldap_servers_ldaps_option_array();
      if (isset($defaults['user']['user_attr']) && ($this->user_attr != $defaults['user']['user_attr'])) {
        $tokens = ['%name' => $defaults['name'], '%default' => $defaults['user']['user_attr'], '%user_attr' => $this->user_attr];
        $warnings['user_attr'] = t('The standard UserName attribute in %name is %default. You have %user_attr. This may be correct
          for your particular LDAP.', $tokens);
      }

      if (isset($defaults['user']['mail_attr']) && $this->mail_attr && ($this->mail_attr != $defaults['user']['mail_attr'])) {
        $tokens = ['%name' => $defaults['name'], '%default' => $defaults['user']['mail_attr'], '%mail_attr' => $this->mail_attr];
        $warnings['mail_attr'] = t('The standard mail attribute in %name is %default. You have %mail_attr. This may be correct
          for your particular LDAP.', $tokens);
      }
    }
    if (!$this->mail_attr && !$this->mail_template) {
      $warnings['mail_attr'] = t('Mail attribute or Mail Template should be used for most user account functionality.');
    }

    return $warnings;
  }

  /**
   *
   */
  public function backdropFormSubmit($op, $values) {

    $this->populateFromBackdropForm($op, $values);

    if ($values['clear_bindpw']) {
      $this->bindpw_clear = TRUE;
    }

    if ($op == 'delete') {
      $this->delete($this);
    }
    // Add or edit.
    else {
      try {
        $save_result = $this->save($op);
      }
      catch (Exception $e) {
        $this->setError('Save Error',
        t('Failed to save object. Your form data was not saved.'));
      }
    }
  }

  /**
   *
   */
  protected function arrayToLines($array) {
    $lines = "";
    if (is_array($array)) {
      $lines = join("\n", $array);
    }
    elseif (is_array(@unserialize($array))) {
      $lines = join("\n", unserialize($array));
    }
    return $lines;
  }

  /**
   *
   */
  protected function linesToArray($lines) {
    $lines = trim($lines);

    if ($lines) {
      $array = preg_split('/[\n\r]+/', $lines);
      foreach ($array as $i => $value) {
        $array[$i] = trim($value);
      }
    }
    else {
      $array = [];
    }
    return $array;
  }

  /**
   *
   */
  public static function fields() {

    /**
     * consumer_type is tag (unique alphanumeric id) of consuming authorization such as
     *   backdrop_roles, og_groups, civicrm_memberships
     */
    $fields = [

      'sid' => [
        'form' => [
          'fieldset' => 'server',
          '#type' => 'textfield',
          '#size' => 20,
          '#title' => t('Machine name for this server configuration.'),
          '#description' => t('May only contain alphanumeric characters (a-z, A-Z, 0-9, and _)'),
          '#required' => TRUE,
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 20,
          'not null' => TRUE,
        ],
      ],

      'numeric_sid' => [
        'schema' => [
          'type' => 'serial',
          'unsigned' => TRUE,
          'not null' => TRUE,
          'description' => 'Primary ID field for the table. Only used internally.',
          'no export' => TRUE,
        ],
      ],

      'name' => [
        'form' => [
          'fieldset' => 'server',
          '#type' => 'textfield',
          '#size' => 50,
          '#title' => 'Name',
          '#description' => t('Choose a <em><strong>unique</strong></em> name for this server configuration.'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'status' => [
        'form' => [
          'fieldset' => 'server',
          '#type' => 'checkbox',
          '#title' => t('Enabled'),
          '#description' => t('Disable in order to keep configuration without having it active.'),
          '#required' => FALSE,
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'tiny',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

      'ldap_type' => [
        'form' => [
          'fieldset' => 'server',
          '#type' => 'select',
          '#options' => ldap_servers_ldaps_option_array(),
          '#title' => t('LDAP Server Type'),
          '#description' => t('This field is informative. It\'s purpose is to assist with default values and give validation warnings.'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 20,
          'not null' => FALSE,
        ],
      ],

      'address' => [
        'form' => [
          'fieldset' => 'server',
          '#type' => 'textfield',
          '#title' => t('LDAP server'),
          '#description' => t('The domain name or IP address of your LDAP Server such as "ad.unm.edu". For SSL
        use the form ldaps://DOMAIN such as "ldaps://ad.unm.edu"'),
          '#size' => 50,
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'port' => [
        'form' => [
          'fieldset' => 'server',
          '#type' => 'textfield',
          '#title' => t('LDAP port'),
          '#size' => 5,
          '#description' => t('The TCP/IP port on the above server which accepts LDAP connections. Must be an integer.'),
        ],
        'schema' => [
          'type' => 'int',
          'not null' => FALSE,
          'default' => 389,
        ],
      ],

      'tls' => [
        'form' => [
          'fieldset' => 'server',
          '#type' => 'checkbox',
          '#title' => t('Use Start-TLS'),
          '#description' => t('Secure the connection between the Backdrop and the LDAP servers using TLS.<br /><em>Note: To use START-TLS, you must set the LDAP Port to 389.</em>'),
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'tiny',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

      'followrefs' => [
        'form' => [
          'fieldset' => 'server',
          '#type' => 'checkbox',
          '#title' => t('Follow LDAP Referrals'),
          '#description' => t('Makes the LDAP client follow referrals (in the responses from the LDAP server) to other LDAP servers. This requires that the Bind Settings you give, is ALSO valid on these other servers.'),
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'tiny',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

      'bind_method' => [
        'form' => [
          'fieldset' => 'bind_method',
          '#type' => 'radios',
          '#title' => t('Binding Method for Searches (such as finding user object or their group memberships)'),
          '#options' => [
            LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT => t('Service Account Bind: Use credentials in the
            <strong>Service Account</strong> field to bind to LDAP. <em>This option is usually a best practice.</em>'),

            LDAP_SERVERS_BIND_METHOD_USER => t('Bind with Users Credentials: Use user\'s entered credentials
            to bind to LDAP.<br/> This is only useful for modules that execute during user logon such
            as LDAP Authentication and LDAP Authorization. <em>This option is not a best practice in most cases.</em>
            This option skips the initial anonymous bind and anonymous search to determine the LDAP user DN, but you
            can only use this option if your user DNs follow a consistent pattern, for example all of them being of
            the form "cn=[username],[base dn]", or all of them being of the form "uid=[username],ou=accounts,[base dn]".
            You specify the pattern under "Expression for user DN" in the next configuration block below.'),

            LDAP_SERVERS_BIND_METHOD_ANON_USER => t('Anonymous Bind for search, then Bind with Users Credentials:
            Searches for user dn then uses user\'s entered credentials to bind to LDAP.<br/> This is only useful for
            modules that work during user logon such as LDAP Authentication and LDAP Authorization.
            The user\'s dn must be discovered by an anonymous search for this option to work.'),

            LDAP_SERVERS_BIND_METHOD_ANON => t('Anonymous Bind: Use no credentials to bind to LDAP server.<br/>
            <em>This option will not work on most LDAPS connections.</em>'),
          ],
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'small',
          'not null' => FALSE,
          'default' => 0,
          'boolean' => FALSE,
        ],
      ],

      'binding_service_acct' => [
        'form' => [
          'fieldset' => 'bind_method',
          '#type' => 'markup',
          '#markup' => t('<label>Service Account</label> Some LDAP configurations
          prohibit or restrict the results of anonymous searches. These LDAPs require a DN//password pair
          for binding. For security reasons, this pair should belong to an
          LDAP account with stripped down permissions.
          This is also required for provisioning LDAP accounts and groups!'),
        ],
      ],

      'binddn' => [
        'form' => [
          'fieldset' => 'bind_method',
          '#type' => 'textfield',
          '#title' => t('DN for non-anonymous search'),
          '#size' => 80,
          '#states' => [
            'enabled' => [
              ':input[name=bind_method]' => ['value' => (string) LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT],
            ],
          ],
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 511,
        ],
      ],

      'bindpw' => [
        'form' => [
          'fieldset' => 'bind_method',
          '#type' => 'password',
          '#title' => t('Password for non-anonymous search'),
          '#size' => 20,
          '#states' => [
            'enabled' => [
              ':input[name=bind_method]' => ['value' => (string) LDAP_SERVERS_BIND_METHOD_SERVICE_ACCT],
            ],
          ],
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
        ],
      ],

      'clear_bindpw' => [
        'form' => [
          'fieldset' => 'bind_method',
          '#type' => 'checkbox',
          '#title' => t('Clear existing password from database. Check this when switching away from Service Account Binding.'),
          '#default_value' => 0,
        ],
      ],

      'basedn' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textarea',
          '#cols' => 50,
          '#rows' => 6,
          '#title' => t('Base DNs for LDAP users, groups, and other entries.'),
          '#description' => '<div>' . t('What DNs have entries relavant to this configuration?
            e.g. <code>ou=campus accounts,dc=ad,dc=uiuc,dc=edu</code>
            Keep in mind that every additional basedn likely doubles the number of queries. Place the
            more heavily used one first and consider using one higher base DN rather than 2 or more lower base DNs.
            Enter one per line in case if you need more than one.') . '</div>',
        ],
        'schema' => [
          'type' => 'text',
          'serialize' => TRUE,
        ],
      ],

      'user_attr' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('AuthName attribute'),
          '#description' => t('The attribute that holds the users\' login name. (eg. <code>cn</code> for eDir or <code>sAMAccountName</code> for Active Directory).'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'account_name_attr' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('AccountName attribute'),
          '#description' => t('The attribute that holds the unique account name. Defaults to the same as the AuthName attribute.'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
          'default' => '',
        ],
      ],

      'mail_attr' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Email attribute'),
          '#description' => t('The attribute that holds the users\' email address. (eg. <code>mail</code>). Leave empty if no such attribute exists'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'mail_template' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Email template'),
          '#description' => t('If no attribute contains the user\'s email address, but it can be derived from other attributes,
            enter an email "template" here.
            Templates should have the user\'s attribute name in form such as [cn], [uin], etc.
            such as <code>[cn]@mycompany.com</code>.
            See https://backdropcms.org/project/ldap#tokens for additional documentation on ldap tokens.
            '),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'picture_attr' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Thumbnail attribute'),
          '#description' => t('The attribute that holds the users\' thumnail image. (eg. <code>thumbnailPhoto</code>). Leave empty if no such attribute exists'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'unique_persistent_attr' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Persistent and Unique User ID Attribute'),
          '#description' => t('In some LDAPs, a user\'s DN, CN, or mail value may
            change when a user\'s name changes or for other reasons.
            In order to avoid creation of multiple accounts for that user or other ambiguities,
            enter a unique and persistent ldap attribute for users. In cases
            where DN does not change, enter "dn" here.
            If no such attribute exists, leave this blank.'
          ),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 64,
          'not null' => FALSE,
        ],
      ],

      'unique_persistent_attr_binary' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'checkbox',
          '#title' => t('Does the <em>Persistent and Unique User ID
            Attribute</em> hold a binary value?'),
          '#description' => t('You need to set this if you are using a binary
             attribute such as objectSid in ActiveDirectory for the PUID.<br>
             If you don\'t want this consider switching to another attribute,
             such as samaccountname.'),
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'tiny',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

      'user_dn_expression' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textfield',
          '#size' => 80,
          '#title' => t('Expression for user DN. Required when "Bind with Users Credentials" method selected.'),
          '#description' => t('%username and %basedn are valid tokens in the expression.
            Typically it will be:<br/> <code>cn=%username,%basedn</code>
             which might evaluate to <code>cn=jdoe,ou=campus accounts,dc=ad,dc=mycampus,dc=edu</code>
             Base DNs are entered above.'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'ldap_to_backdrop_user' => [
        'form' => [
          'fieldset' => 'users',
          '#disabled' => (!module_exists('php')),
          '#type' => 'textarea',
          '#cols' => 25,
          '#rows' => 5,
          '#title' => t('PHP to transform Backdrop login username to LDAP UserName attribute.'),
          '#description' => t('This will appear as disabled unless the "PHP filter" core module is enabled. Enter PHP to transform Backdrop username to the value of the UserName attribute.
            The code should print the UserName attribute.
            PHP filter module must be enabled for this to work.
            The variable $name is available and is the user\'s login username.
            Careful, bad PHP code here will break your site. If left empty, no name transformation will be done.
            <br/>Example:<br/>Given the user will logon with jdoe@xyz.com and you want the ldap UserName attribute to be
            jdoe.<br/><code>$parts = explode(\'@\', $name); if (count($parts) == 2) {print $parts[0]};</code>'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 1024,
          'not null' => FALSE,
        ],
      ],

      'testing_backdrop_username' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Testing Backdrop Username'),
          '#description' => t('This is optional and used for testing this server\'s configuration against an actual username. The user need not exist in Backdrop and testing will not affect the user\'s LDAP or Backdrop Account.'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'testing_backdrop_user_dn' => [
        'form' => [
          'fieldset' => 'users',
          '#type' => 'textfield',
          '#size' => 120,
          '#title' => t('DN of testing username, e.g. cn=hpotter,ou=people,dc=hogwarts,dc=edu'),
          '#description' => t('This is optional and used for testing this server\'s configuration against an actual username. The user need not exist in Backdrop and testing will not affect the user\'s LDAP or Backdrop Account.'),
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'grp_unused' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'checkbox',
          '#title' => t('Groups are not relevant to this Backdrop site. This is generally true if LDAP Groups, LDAP Authorization, etc are not it use.'),
          '#disabled' => FALSE,
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'tiny',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

      'grp_object_cat' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Name of Group Object Class'),
          '#description' => t('e.g. groupOfNames, groupOfUniqueNames, group.'),
          '#states' => [
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 64,
          'not null' => FALSE,
        ],
      ],

      'grp_nested' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'checkbox',
          '#title' => t('Nested groups are used in my LDAP'),
          '#disabled' => FALSE,
          '#description' => t('If a user is a member of group A and group A is a member of group B,
             user should be considered to be in group A and B. If your LDAP has nested groups, but you
             want to ignore nesting, leave this unchecked.'),
          '#states' => [
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'tiny',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

      'grp_user_memb_attr_exists' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'checkbox',
          '#title' => t('A user LDAP attribute such as <code>memberOf</code> exists that contains a list of their groups.
            Active Directory and openLdap with memberOf overlay fit this model.'),
          '#disabled' => FALSE,
          '#states' => [
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'tiny',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

      'grp_user_memb_attr' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Attribute in User Entry Containing Groups'),
          '#description' => t('e.g. memberOf'),
          '#states' => [
            'enabled' => [
              ':input[name=grp_user_memb_attr_exists]' => ['checked' => TRUE],
            ],
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'grp_memb_attr' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('LDAP Group Entry Attribute Holding User\'s DN, CN, etc.'),
          '#description' => t('e.g uniquemember, memberUid'),
          '#states' => [
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'grp_memb_attr_match_user_attr' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('User attribute held in "LDAP Group Entry Attribute Holding..."'),
          '#description' => t('This is almost always "dn" (which technically isn\'t an attribute). Sometimes its "cn".'),
          '#states' => [
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'grp_derive_from_dn' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'checkbox',
          '#title' => t('Groups are derived from user\'s LDAP entry DN.') . ' <em>' .
          t('This
            group definition has very limited functionality and most modules will
            not take this into account. LDAP Authorization will.') . '</em>',
          '#disabled' => FALSE,
          '#states' => [
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'tiny',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

      'grp_derive_from_dn_attr' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'textfield',
          '#size' => 30,
          '#title' => t('Attribute of the User\'s LDAP Entry DN which contains the group'),
          '#description' => t('e.g. ou'),
          '#states' => [
            'enabled' => [
              ':input[name=grp_derive_from_dn]' => ['checked' => TRUE],
            ],
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'grp_test_grp_dn' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'textfield',
          '#size' => 120,
          '#title' => t('Testing LDAP Group DN'),
          '#description' => t('This is optional and can be useful for debugging and validating forms.'),
          '#states' => [
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'grp_test_grp_dn_writeable' => [
        'form' => [
          'fieldset' => 'groups',
          '#type' => 'textfield',
          '#size' => 120,
          '#title' => t('Testing LDAP Group DN that is writable. WARNING the test script for the server will create, delete, and add members to this group!'),
          '#description' => t('This is optional and can be useful for debugging and validating forms.'),
          '#states' => [
            'visible' => [
              ':input[name=grp_unused]' => ['checked' => FALSE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'varchar',
          'length' => 255,
          'not null' => FALSE,
        ],
      ],

      'search_pagination' => [
        'form' => [
          'fieldset' => 'pagination',
          '#type' => 'checkbox',
          '#title' => t('Use LDAP Pagination.'),
          '#disabled' => !ldap_servers_php_supports_pagination(),
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'tiny',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

      'search_page_size' => [
        'form' => [
          'fieldset' => 'pagination',
          '#type' => 'textfield',
          '#size' => 10,
          '#disabled' => !ldap_servers_php_supports_pagination(),
          '#title' => t('Pagination size limit.'),
          '#description' => t('This should be equal to or smaller than the max
            number of entries returned at a time by your ldap server.
            1000 is a good guess when unsure. Other modules such as LDAP Query
            or LDAP Feeds will be allowed to set a smaller page size, but not
            a larger one.'),
          '#states' => [
            'visible' => [
              ':input[name="search_pagination"]' => ['checked' => TRUE],
            ],
          ],
        ],
        'schema' => [
          'type' => 'int',
          'size' => 'medium',
          'not null' => FALSE,
          'default' => 1000,
        ],
      ],

      'weight' => [
        'schema' => [
          'type' => 'int',
          'not null' => FALSE,
          'default' => 0,
        ],
      ],

    ];

    return $fields;

  }

}

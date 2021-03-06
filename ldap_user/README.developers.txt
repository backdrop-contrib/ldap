
provisioning = creating or synching ... to Backdrop or to ldap



==========================================
LDAP User Data Structures in Backdrop User Object
==========================================


'data' => 
  array (
    'ldap_user' => 
    array (
      'init' => 
      array (
        'sid' => 'activedirectory1',
        'dn' => 'cn=hpotter,ou=people,dc=hogwarts,dc=edu',
        'mail' => 'hpotter@hogwarts.edu',
      ),
    ),
    'ldap_authorizations' => 
    array (
      'backdrop_role' =>
      array (
        'cn=gryffindor,ou=groups,dc=hogwarts,dc=edu' => 
        array (
          'date_granted' => 1351194052,
        ),
        'cn=honors students,ou=groups,dc=hogwarts,dc=edu' => 
        array (
          'date_granted' => 1351194052,
        ),
        'students' => 
        array (
          'date_granted' => 1351194052,
        ),
      ),
    ),
  ),

 'ldap_user_puid_sid' => 
  array (
    LANGUAGE_NONE =>
    array (
      0 => 
      array (
        'value' => 'activedirectory1',
        'format' => NULL,
        'safe_value' => 'activedirectory1',
      ),
    ),
  ),
   'ldap_user_puid' => 
  array (
    LANGUAGE_NONE =>
    array (
      0 => 
      array (
        'value' => '101',
        'format' => NULL,
        'safe_value' => '101',
      ),
    ),
  ),
   'ldap_user_puid_property' => 
  array (
    LANGUAGE_NONE =>
    array (
      0 => 
      array (
        'value' => 'guid',
        'format' => NULL,
        'safe_value' => 'guid',
      ),
    ),
  ),
   'ldap_user_current_dn' => 
  array (
    LANGUAGE_NONE =>
    array (
      0 => 
      array (
        'value' => 'cn=hpotter,ou=people,dc=hogwarts,dc=edu',
        'format' => NULL,
        'safe_value' => 'cn=hpotter,ou=people,dc=hogwarts,dc=edu',
      ),
    ),
  ),
   'ldap_user_prov_entries' => 
  array (
  ),
   'ldap_user_last_checked' => 
  array (
  ),
   'ldap_authorizations' => 
  array (
  ),


==========================================
hooks relating ldap_user entities and backdrop user entities
==========================================

- hook_user_create, hook_user_update, hook_user_delete should look for ldap_user entity with matching uid and deal with ldap_user entity appropriately.
- hook_ldap_user_create, hook_ldap_user_update, hook_ldap_user_delete should do appropriate things to user entity.


==========================================
Rough Summary of provisioning configuration and controls
==========================================

1. configured triggers (admin/config/people/ldap/user) or configuration of other modules
determine when provisioning happens.

// configurable Backdrop acct provision triggers
LDAP_USER_BACKDROP_USER_PROV_ON_USER_UPDATE_CREATE
LDAP_USER_BACKDROP_USER_PROV_ON_AUTHENTICATE
LDAP_USER_BACKDROP_USER_PROV_ON_ALLOW_MANUAL_CREATE

// configurable ldap entry provision triggers 
LDAP_USER_LDAP_ENTRY_PROV_ON_USER_UPDATE_CREATE
LDAP_USER_LDAP_ENTRY_PROV_ON_AUTHENTICATE
LDAP_USER_LDAP_ENTRY_DELETE_ON_USER_DELETE


2. hook_user_* functions (and elsewere such as ldap_authentication) will check if appropriate triggers are enabled and initiate calls to ldapUserConf methods:

ldapUserConf::provisionBackdropAccount()
ldapUserConf::synchToBackdropAccount()
ldapUserConf::ldapAssociateBackdropAccount()
ldapUserConf::deleteBackdropAccount()

ldapUserConf::provisionLdapEntry()
ldapUserConf::synchToLdapEntry()
ldapUserConf::deleteProvisionedLdapEntries()


3. to get mappings and determine which attributes are needed "ldap_contexts" and "prov_events" are passed into 
ldap_servers_get_user_ldap_data()
ldapUserConf::backdropUserToLdapEntry()


4. Should provisioning happen?

------------
4.A. Server Level: Does an ldap server configuration support provisioning?
ldapUserConf::backdropAcctProvisionServer = <sid> | LDAP_USER_NO_SERVER_SID;  // servers used for to Backdrop acct provisioning
ldapUserConf::ldapEntryProvisionServer =  <sid> | LDAP_USER_NO_SERVER_SID;  // servers used for provisioning to ldap

This is directly configured at config/people/ldap/user

------------
4.B. Trigger Level: Does provisioning occur for a given trigger?
ldapUserConf::provisionEnabled($direction, $provision_trigger)
    
This method is based on the configuration of two sets of checkboxes at config/people/ldap/user

ldapUserConf::backdropAcctProvisionTriggers (see "LDAP Entry Provisioning Options"), contains:
  LDAP_USER_BACKDROP_USER_PROV_ON_AUTHENTICATE
  LDAP_USER_BACKDROP_USER_PROV_ON_USER_UPDATE_CREATE
  LDAP_USER_BACKDROP_USER_PROV_ON_ALLOW_MANUAL_CREATE

ldapUserConf::ldapEntryProvisionTriggers (see "Backdrop Account Provisioning Options"), contains:
  LDAP_USER_LDAP_ENTRY_PROV_ON_USER_UPDATE_CREATE
  LDAP_USER_LDAP_ENTRY_DELETE_ON_USER_DELETE
  LDAP_USER_LDAP_ENTRY_PROV_ON_AUTHENTICATE

@todo. A hook to allow other modules to intervene here

------------
4.C Field Level: Does provisioning occur for a given field and ldap server for a given "prov_event" and "ldap _context"?

ldapUserConf::isSynched($field, $prov_event, $direction)

This depends on: 
ldapUserConf::synchMapping[$direction][$field]['prov_events']
which is populated by various ldap and possibly other modules.

"ldap_contexts" (any module can provide its own context which is just a string)
  ldap_user_insert_backdrop_user
  ldap_user_update_backdrop_user
  ldap_authentication_authenticate
  ldap_user_delete_backdrop_user
  ldap_user_disable_backdrop_user
  all

"prov_events"
  LDAP_USER_EVENT_SYNCH_TO_BACKDROP_USER
  LDAP_USER_EVENT_CREATE_BACKDROP_USER
  LDAP_USER_EVENT_SYNCH_TO_LDAP_ENTRY
  LDAP_USER_EVENT_CREATE_LDAP_ENTRY
  LDAP_USER_EVENT_LDAP_ASSOCIATE_BACKDROP_ACCT

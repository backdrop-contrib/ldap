Lightweight Directory Access Protocol (LDAP) module
===================================================

The Lightweight Directory Access Protocol (LDAP) project provides integration
with LDAP for authentication, user provisioning, authorization, queries, feeds,
and views. It also provides API and building blocks (query and server
configuration storage) for other modules. It allows you to integrate your
organization's existing LDAP-enabled identity management service (such as
Active Directory) into Backdrop CMS.

Content
-------

### Modules

- [LDAP guides](#ldap_guides)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [LDAP Servers module](#servers)
- [LDAP User module](#user)
- [LDAP Authentication module](#authentication)
- [LDAP Authorization module](#authorization)
  - [Vocabulary of LDAP Authorization](#vocabulary)
  - [Use cases](#authusecases)
  - [Derive Authorizations from User DN](#derive1)
  - [Derive Authorizations from User Attribute](#derive2)
  - [Derive Authorizations from LDAP Group Entry](#derive3)
  - [LDAP Authorization - Backdrop Roles module](#roles)
  - [LDAP Authorization - OG (Organic Groups) module](#og)
- [LDAP Query module](#query)
- [LDAP Views module](#views)
- [LDAP Feeds module](#feeds)
  - [Example use-cases](#feedsusescases)
  - [Detailed example 1.](#feedsexample1)
  - [Detailed example 2.](#feedsexample2)
- [LDAP Test module](#test)
- [LDAP Help module](#help)
- [LDAP SSO module](#sso)
- [Tokens](#tokens)


### Testing

- [Automated testing](#automatedtesting)
- [Manual testing](#manualtesting)
- [Local LDAP test server by Vagrant](#vagranttesting)
  - [1. Building a test environment](#buildingvagranttesting)
  - [2. Configure the LDAP module for the local test server](#configvagranttesting)
  - [3. Manual tests of the LDAP module on the local test server](#manualvagranttesting)
- [LDAP test script without Backdrop](#testscript)
- [Debugging](#debugging)


### Other information

- [Issues](#issues)
- [Current Maintainer](#maintainer)
- [Credits](#credits)
- [License](#license)
- [Screenshot](#screenshot)





LDAP guides <a name="ldap_guides"></a>
-----------

Many problems in setting up LDAP for Backdrop stem from issues outside of
Backdrop CMS and are much easier to debug outside of it, you might find these
guides helpful:
- Moodle's [LDAP module documentation](https://docs.moodle.org/20/en/LDAP_authentication) is
detailed and provides insight into LDAP in a PHP environment.
- OpenLDAP TLS/LDAPS setup: https://www.openldap.org/faq/data/cache/185.html
- Documentation from the PHP project on its [LDAP implemtation](https://secure.php.net/manual/en/book.ldap.php)
- Microsoft's Active Directory [documentation overview](https://msdn.microsoft.com/en-us/library/aa705886(VS.85).aspx)
- [Apache Directory Studio](https://directory.apache.org/studio/) LDAP Browser and Directory Client.
- [Novell Edirectory](https://www.netiq.com/documentation/edirectory-92/)

#### Example documentation:

- [Northwestern University](http://www.it.northwestern.edu/bin/docs/CentralAuthenticationServicesThroughLDAP.pdf)



Prerequisites <a name="prerequisites"></a>
-------------

The following requirements need to be met for you to work with any of the LDAP
modules:

- PHP version 5.4
- PHP LDAP extension (php_ldap)

Verify the following:
- mcrypt extension is loaded if you are going to encrypt stored passwords.
- OpenSSL or other SSL extension is loaded. (php_openssl)
- For ldaps make sure certificate is valid on webserver.

#### Getting the relevant information <a name="getting_the_relevant_information"></a>

To set up LDAP efficiently, you need to get the relevant information of your
environment from your directory administrator before you can continue.
This should include:
- The servers available to you (hostname, port, encryption preference)
- The binding method (service account including credentials, if necessary)
- If applicable, the structure of the data you are trying to sync
  e.g. sAMAccountName is the unique name attribute for your Active Directory.

We have prepared the following sample letter you can send to the responsible
party to receive the relevant data:

>    Dear [LDAP|Active Directory] Administrator,
>
>    We would like to leverage the [Campus|Corporate|etc] [LDAP|Active Directory]
>    for authentication and authorization on our Backdrop CMS website. It will be
>    used in the following way:
>       - Users will enter their credentials in the Backdrop interface and Backdrop
>         will test them against the LDAP by binding with them.
>       - A mirrored Backdrop Account will also be created with their email and a
>         long random password; no LDAP credentials will be stored in Backdrop.
>       - LDAP Groups will be mirrored with Backdrop roles and Backdrop role
>         memberships will be derived for LDAP Groups and OUs.
>
>    We have the following questions about configuration and best practices.
>    Whatever you can tell us will be helpful. Once we get connected to the LDAP
>    server, we can hopefully figure out any missing pieces.
>    LDAP Server Connection Properties:
>      - What type of LDAP is it (Active Directory, Open LDAP, Open Directory,
>        eDirectory, etc)?
>      - Should we bind with a service account for querying user attributes and
>        group memberships? Or use an anonymous bind?
>          If so do we create the service account or can you?
>          If you create it, what is the Distinguished Name (eg. cn=jdoe,ou=...)
>          for it and password?
>      - What is the base distinguished name that we should bind to? We suspect
>        it's the top-level DN, but anything above the users and group OUs should
>        work.
>      - What is the LDAP server host name and port (e.g. ad.mycompany.com:386)?
>      - Should we connect with StartTLS, or ldaps, or neither?
>      - Are there any firewall issues we need to resolve to connect from our
>        web server to the LDAP server?
>
>    LDAP User Entries:
>      - What attribute contains the users email address (e.g. mail)?
>      - Is there a unique attribute such as uid, guid, etc. that does not change
>        over time?
>      - What attribute would make a good logon/username (e.g. "cn")?
>
>    Group Entries:
>      - Does the user's LDAP entry have an attribute such as memberOf that
>        contains the user's group memberships?
>      - What attribute in the group ldap entries holds the users (e.g. uniquemember,
>        memberUid)? And what is held in this attribute (DN, CN, uid, ..)?
>      - What is the object class of the group entries (e.g. groupOfNames,
>        groupOfUniqueNames, group)?
>
>    Thanks ...



Installation <a name="installation"></a>
------------

- Install this module using the official Backdrop CMS instructions at
  https://backdropcms.org/guide/modules

- Enable the "LDAP Servers" module from the LDAP group, and configure at least
  one server:
  Administration > Configuration > User accounts > LDAP Configuration > Servers
  ( admin/config/people/ldap/servers )

- Enable the other LDAP modules you need. See below.

- Configuration page:
  Administration > Configuration > User accounts > LDAP Configuration
  ( admin/config/people/ldap )


#### Configuration overrides

If you need to selectively disable LDAP functionality and cannot disable the
modules, use configuration overrides, such as the following in settings.php and
clearing your cache afterwards.

- Disable the server you are syncing users from LDAP to Backdrop.
```
$conf['ldap_user_conf']['backdropAcctProvisionServer'] = 0;
```

- Disable LDAP authentication.
```
$conf['ldap_authentication_conf']['sids'] = [];
```

- Set bind DN and bind password for service account.
```
$conf['ldap_servers_overrides']['my_server']['binddn'] = 'my_dn';
$conf['ldap_servers_overrides']['my_server']['bindpw'] = 'my_password';
```

IMPORTANT: These overrides will change the data in your admin forms, saving them
will save them permanently in the database.

**The project consists of 11 modules:**


LDAP Servers module <a name="servers"></a>
-------------------

The base module for communicating with a directory. Required by all other LDAP modules.
Implements two configuration pages:
- Settings page:
  - Administration > Configuration > User accounts > LDAP Configuration
    ( admin/config/people/ldap )
  - Storing LDAP passwords and development settings.
  
- LDAP servers configuration page:
  - Administration > Configuration > User accounts > LDAP Configuration > Servers
    ( admin/config/people/ldap/servers )
  - Operations:
    - Add an LDAP server configuration: server type, server address, port,
      bindig method, LDAP user to Backdrop user relationship, group
      configuration, pagination etc.
    - Edit, delete or disable an LDAP server configuration
    - Test: bindig, group create, delete, add member, remove member


LDAP User module <a name="user"></a>
----------------

The core functionality of this module is provisioning and storage of an LDAP
identified Backdrop user based on LDAP attributes. The LDAP User module is used
to relate, provision (create), and synchronize attributes of LDAP user entries
and Backdrop users. Provisioning and synching can go from LDAP to Backdrop and
from Backdrop to LDAP. LDAP User module leverages LDAP Server module which
configures LDAP server connections and other LDAP server specific data.
(provisioning: creating or synching to Backdrop or to LDAP)

Dependencies:
- LDAP Servers module
- Number module

Configuration page:
- Administration > Configuration > User accounts > LDAP Configuration > User
  ( admin/config/people/ldap/user )
- How to resolve LDAP conflicts with manually created Backdrop accounts
- Basic Provisioning to Backdrop Account Settings
- Basic Provisioning to LDAP Settings
- Test LDAP user functionality for a given user

(LDAP User module configuration depends on LDAP Servers configuration and other
modules such as LDAP Authentication for some use cases.)

#### Provisioning Backdrop Users from LDAP User Entries

Use cases:
- Your organization has an LDAP with its users in it. You want username,
  emails, names, etc. to be automatically populated in your Backdrop accounts.
- You want your user to authenticate to Backdrop with their LDAP credentials.
  (This requires LDAP Authentication module enabled).
- You do not want to create accounts until users start using your Backdrop site.
  If you want to sync all LDAP users to Backdrop, see
  LDAP Feeds Example: [Detailed example 2.](#feedsexample2)
- You want to associate existing user with LDAP: Go to admin/config/people/ldap/user :
  "How to resolve LDAP conflicts with manually created Backdrop accounts."

#### Events to Provisioning Backdrop Users from LDAP User Entries

The actual creation of the Backdrop account can happen:
- On user logon via LDAP authentication. This is the most common use case.
  After the user successfully authenticates, a Backdrop account is created.
  Fields in the Backdrop account (username, mail, uid, last name, etc.) are
  populated based on LDAP User mapping configurations.
- On manual Backdrop Account Creation. For this use case, whenever a Backdrop
  account is created a check is done for a corresponding LDAP entry. If one
  is found, the Backdrop account fields are populated from the LDAP Entry.
  This is useful when you have few users and you want to create accounts by
  hand. Or when you are using other modules to mass import users.
- On any Backdrop Account Creation. Regardless of how Backdrop account is created.
- On cron runs. (Not implemented)
- On REST webservice Request. (Not implemented)

#### Provisioning LDAP Entries from Backdrop Users

Use cases:
- Your organization wants to use Backdrop as your account management tool, but
  wants to leverage other web applications. Since LDAP is a standard for both
  authentication and user data, it can be leveraged by CAS, WordPress, Jira, etc.
- Your organization has an LDAP with all of its internal members, but you want
  to have an OU for "external" users. You leverage Backdrop's self service accounts
  and have LDAP entries created for each ldap user so they can gain access to
  other resources. Your internal users use their existing LDAP account
  credentials and don't need to maintain a second set of credentials.
- Synching LDAP Entry Attributes from Backdrop Users Fields
- Edit LDAP identified user (go directly to ldap_user entity and edit and/or add
  link to edit ldap_user to user forms).

#### Diagrams

[Sequence Diagram](https://github.com/backdrop-contrib/ldap/blob/1.x-2.x/images/sequence-diagram.png) of `$ldapUserConf->provisionBackdropAccount()` method.
([Sequence Diagram](http://www.gliffy.com/go/publish/3664260/) on gliffy.com)



LDAP Authentication module <a name="authentication"></a>
--------------------------

This module provides an overall authentication functionality closely tied to
ldap_user and ties in with several other modules, such as LDAP SSO (ldap_sso).

Dependencies:
- LDAP Servers module
- LDAP User module

Configuration page:
- Administration > Configuration > User accounts > LDAP Configuration > Authentication
  ( admin/config/people/ldap/authentication )
- Logon Options:
  - Allowable Authentications: Mixed mode or only LDAP authentication is allowed
  - LDAP server configurations to use in authentication
- User login interface
- LDAP User "whitelists" and restrictions
- User's email address: behavior, update, templates, User email prompt
- Password behavior
- Single Sign-On (Note that this module is a [separate project on backdropcms.org](https://www.backdropcms.org/project/ldap_sso).)



LDAP Authorization module <a name="authorization"></a>
--------------------------

LDAP Authorization module to grant roles to users based on directory criteria.
It is simply an API for "authorization consumers" such as Backdrop roles or
Organic Groups groups. Backdrop roles is most commonly used. You must enable
LDAP Authorization and one or more "authorization consumer" modules.
Each "authorization consumer" will have a single configuration entry at:
admin/config/people/ldap/authorization that will need to be created,
configured and enabled for authorization to work.

After configuring an "authorization consumer", use the "test" link to see
the authorizations a given test user would be granted.

Dependencies:
- LDAP Servers module
- LDAP User module
- LDAP Authorization - Backdrop Roles module or/and LDAP Authorization - OG (Organic Groups) module
  (These are submodules of LDAP Authorization module.)

Configuration page:
- Administration > Configuration > User accounts > LDAP Configuration > Authorization
  ( admin/config/people/ldap/authorization )
- Operations:
  - Add an "authorization consumer" (Backdrop role or Organic Group group):
    - Select LDAP Server used in consumer configuration.
    - LDAP to consumer's authorization mapping and filtering.
    - When should consumer's authorizations be granted/revoked from user?
    - What actions would you like performed when consumer's authorizations
      are granted/revoked from user?
  - Edit, delete a consumer.
  - Test: It shows what authorizations would be granted with this configuration.


#### Vocabulary of LDAP Authorization and its Code: <a name="vocabulary"></a>

- Consumer: The "consumer" or entity that authorization is being granted.
  Examples:  Backdrop role, Organic Group group
- Consumer Type: Machine ID of a consumer. This is used in naming conventions.
  Examples: backdrop_role, og_group
- Consumer Module: The module that bridges ldap_authorization and the consumer.
  It needs to (1) provide a class: LdapAuthorizationConsumer<consumer_type>
  and (2) implement hook_ldap_authorization_consumer.
  Examples:  ldap_authorization_backdrop_role
- Authorization ID aka Consumer ID: The id of an individual authorization such
  as a Backdrop role or organic group.
  Examples:  "authenticated user", "admin" (for Backdrop roles)
  Examples:  "knitters on skates", "vacationing programmers" (og group names
  for Organic Groups)
- Consumer Configuration: Configuration of how a users LDAP attributes will
  determine a set of Consumer ids the user should be granted.
  Represented by LdapAuthorizationConsumerConf and
  LdapAuthorizationConsumerConfAdmin classes and managed at
  /admin/config/people/ldap/authorization
  Stored in ldap_authorization database table.
- LDAP Server Configuration: Each Consumer Configuration will use a single
  LDAP server configuration to bind and query LDAP. The LDAP server
  configuration is also used to map the Backdrop username to an LDAP user entry.
- LDAP Authorization data storage: Authorization data is stored in user->data array
  such as:
  ```
  $user->data = array(
    'ldap_authorizations' => array(
      'og_group' => array (
        '3-2' => array (
          'date_granted' => 1329105152,
        ),
        '2-3' => array (
          'date_granted' => 1329105152,
        ),
      ),
      'backdrop_role' => array (
        '7' => array (
          'date_granted' => 1329105152,
        ),
        '5' => array (
          'date_granted' => 1329105152,
        ),
      ),
    );
  ```
#### Use cases <a name="authusecases"></a>

Use cases are many and varied so the user interface that accommodates them
can be complex.

Detailed explanations of the three LDAP group mapping strategies are described
in below.

##### Active Directory + Groups Example

Goal: Have users log in to your Backdrop website based on their credentials from
ActiveDirectory, ensure that their 'group' in ActiveDirectory puts them in the
correct Backdrop 'role'.

In this example we created groups in ActiveDirectory and added users to those
role(s), the base DN for the AD/LDAP Server is "dc=drupal,dc=local". I now have
a OrganisationalUnit (ou) called "webadmin" and a Group called "drupal".

![AD layout](https://github.com/backdrop-contrib/ldap/blob/1.x-2.x/images/ad.png)

Using the ['Active Directory Explorer'](https://technet.microsoft.com/en-us/library/bb963907.aspx) application my new user 'joe' looks like the following
(once he has been assigned to the new group).

![Joe config](https://github.com/backdrop-contrib/ldap/blob/1.x-2.x/images/adconfig.png)

##### Backdrop basics

- Ensure your ActiveDirectory server is accessible and running correctly.
- Be sure your PHP is running php_ldap.
- Enable these modules: ldap_user, ldap_servers, ldap_authentication,
  ldap_authorization, ldap_authorization_backdrop_roles

##### Go to admin/config/people/ldap/servers

- Configure a new server, select Active Directory and set the obvious values.

Important parts here:
- Select "Bind with Users Credentials".
- "Base DNs for LDAP users, groups, and other entries" is "CN=Users, DC=drupal, DC=local".
- "AuthName attribute" is "cn".
- "AccountName attribute" is "cn".
- "Email attribute" is "mail".
- "Expression for user DN" is "cn=%username,%basedn".
- "Name of Group Object Class" is "group".
- "A user LDAP attribute such as memberOf exists that contains a list of their
  groups" is CHECKED.
- "Attribute in User Entry Containing Groups" is "memberof".
- "LDAP Group Entry Attribute Holding User's DN, CN, etc." is "distinguishedname".
- "User attribute held in LDAP Group Entry Attribute Holding..." is "dn".
- "Groups are derived from user's LDAP entry DN" is CHECKED.
- "Attribute of the User's LDAP Entry DN which contains the group" is "OU".

##### Go to admin/config/people/ldap/user

- Be sure to select your server at "LDAP Servers Providing Provisioning Data".
- "Create or Synch to Backdrop user on successful authentication with LDAP credentials"
  is CHECKED.
- Select your server at "LDAP Servers to Provision LDAP Entries on".

##### Go to admin/config/people/ldap/authorization/edit/backdrop_role

This is where we do the actual mapping between LDAP/AD 'Groups' and Backdrop 'Roles':
- Add a new authorization mechanism.
- Select your LDAP server.
- "Enable this configuration" is CHECKED.
- "Only apply the following LDAP to Backdrop role configuration to users
  authenticated via LDAP" is up to you, in our case it is CHECKED.
- "Only grant drupal roles that match a filter above" is CHECKED.

Now for the fun part..

##### Mapping of LDAP to Backdrop role (one per line)

There is a Backdrop Role called 'editor', and I want anyone in the LDAP group
'drupal' of the organisational unit webadmin to be assigned to this group
when they authenticate, my example config is just:
`CN=drupal,OU=webadmin,DC=drupal,DC=local|editor`

![roles mapping configuration](https://github.com/backdrop-contrib/ldap/blob/1.x-2.x/images/mapping.png)

Taddaa, now the user logs in with the role automatically assigned:

![Joe user logged in](https://github.com/backdrop-contrib/ldap/blob/1.x-2.x/images/assigned.png)

#### Derivation strategies of the authorizations
Set the derivation at the server's configuration page:
Administration > Configuration > User accounts > LDAP Configuration > Servers > edit

Strategies:
- Derive Authorizations from User Attribute: options A.
- Derive Authorizations from LDAP Group Entry: options B.
- Derive Authorizations from User DN: options C.

![Derivation strategies](https://github.com/backdrop-contrib/ldap/blob/1.x-2.x/images/server_group_config.png)

#### Derive Authorizations from User DN <a name="derive1"></a>

##### How "Derive from user DN" Works:

1. Query for user's ldap entry.
   e.g. cn=verykool, ou=sysadmins, ou=it,dc=ad,dc=myuniversity,dc=edu
2. Whichever attribute (e.g. ou) listed in "Attribute of the User's LDAP Entry
   DN which contains the group" (options C.), will have its value added to the
   list of authorizations. E.g. `sysadmins` and `it`
3. "Derive from user DN" does not support nested groups. Nested has no meaning
   in this approach.

##### What an LDAP looks like that can use the "Derive from user DN" approach.

This can be useful in any LDAP and is typically used with one of the other two
approaches at the same time. While options B. and C. are designed for
two different LDAP group models, "Derive from user DN" simply leverages user DN
attributes such as "ou" which may map to authorizations.


#### Derive Authorizations from User Attribute <a name="derive2"></a>

##### How "Derive by Attribute" Works:
- Query for user's ldap entry.
- The attribute listed in "Attribute in User Entry Containing Groups"
  (options A.), adds entries to list of authorizations.
- If nested is selected keep finding parent groups recursively.

##### Which LDAPs should use the "Derive by Attribute" approach?

Microsoft's Active Directory has this structure. The user's attribute "memberOf"
will have a list of all the groups the user is a member of.

##### What nested groups mean in "Derive by Attribute" approach.

In this approach, nested groups means taking all the groups in memberOf and
adding the groups they belong to, recursively. That is if jdoe belongs to the
bakers group and the bakers group is member of the "food workers" group, jdoe's
authorizations will include bakers and "food workers"

##### Step by Step walkthrough

Configuration and Sample Data (for further understanding):

- User `verykool` has an ldap entry of:
    ```
    'dn' => 'cn=verykool,ou=it,dc=ad,dc=myuniveristy,dc=edu',
    'mail' => array( 0 => 'verykool@myuniversity.edu', 'count' => 1),
    'sAMAccountName' => array( 0 => 'verykool', 'count' => 1),
    'password' => array( 0 => 'goodpwd', 'count' => 1),
    'memberOf' => array(
      0 => 'cn=sysadmins,ou=it,dc=ad,dc=myuniveristy,dc=edu',
      1 => 'CN=NETadmins,ou=it,dc=ad,dc=myuniveristy,dc=edu',
      2 => 'cn=phone operators,ou=it,dc=ad,dc=myuniveristy,dc=edu',
      'count' => 3,
      ),
    ```
- options A: Derive from attributes is checked and "memberOf" is in the
  "Attribute in User Entry Containing Groups" text area.
- "Name of Group Object Class" is set to "group".

###### Step through for example above:

1. LDAP Authorization finds the user entry and checks if any "memberOf"
   attributes exist.
2. It loops through all of the memberOf attributes and adds each to the
   authorizations/groups list. e.g.
   `cn=sysadmins,ou=it,dc=ad,dc=myuniveristy,dc=edu`,
   `CN=NETadmins,ou=it,dc=ad,dc=myuniveristy,dc=edu`,
   `cn=phone operators,ou=it,dc=ad,dc=myuniveristy,dc=edu`,
3. If "Nested groups are used in my LDAP" is checked, a search is performed
   for all groups that have these groups as members (their parents).
    ```
    (&
      (objectClass=group)
      (|
       (memberOf=cn=sysadmins,ou=it,dc=ad,dc=myuniveristy,dc=edu)
       (memberOf=CN=NETadmins,ou=it,dc=ad,dc=myuniveristy,dc=edu)
       (memberOf=cn=phone operators,ou=it,dc=ad,dc=myuniveristy,dc=edu)
      )
    )
    ```
    
    The memberOf attributes from the resulting groups are added to the
    authorizations. This continues on recursively until no results are found
    or a limit of 10 nests is reached. Because of the number of queries
    involved, it is best to use 1 high level basedn instead of several lower
    ones.
4. If "Convert full dn to value of first attribute" is checked at
   admin/config/people/ldap/authorization/edit/backdrop_role, the entire array
   of dns is converted to first attribute. e.g.
   `cn=sysadmins,ou=it,dc=ad,dc=myuniveristy,dc=edu` becomes `sysadmins`.
   This option is problematic when many groups are involved and name collisions
   may occur.


#### Derive Authorizations from LDAP Group Entry <a name="derive3"></a>

##### How "Derive from Entry" Works:
1. Options B: The user attribute and the group entry attribute holding user's
   attribute are configured.
2. Each of the groups that has the user attribute in question as a member is
   added to authorizations.
3. If nested is selected keep finding child groups recursively. If user is a
   member of a child group, the ancestor is added to authorizations.

##### Which LDAPs should use the "Derive from Entry" approach?

This scenario is most applicable to UNIX LDAP environments. In this scenario,
the LDAP groups are stored as objects with their members represented by a
mulitvalued attribute. That attribute's name might be: member, members,
memberUid, uniquemember, etc. That attribute's value might be the DN or CN of
another group or user. (LDAPs that use the memberOf overlay, should use
options A.)

##### What nested groups mean in "Derive from Entry" approach.

If user is a member of a child group, the ancestor in is added to
authorizations. That is if jdoe belongs to the bakers group and the bakers
group is member of the "food workers" group, jdoe's authorization will be
"food workers".

##### Step by Step walkthrough

Sample data and configuration (for further understanding):
- The following group entries exist in LDAP:
    ```
    'dn' => 'cn=developers,cn=groups,dc=ad,dc=myuniversity,dc=edu',
    'objectclass' => array( 0 => 'groupofuniquenames', 'count' => 1),
    'uniquemember' => array(
      0 => 'uid=joeprogrammer,ou=it,dc=ad,dc=myuniversity,dc=edu',
    ),

    'dn' => 'cn=it,cn=groups,dc=ad,dc=myuniversity,dc=edu',
    'objectclass' => array( 0 => 'groupofuniquenames', 'count' => 1),
    'uniquemember' => array(
      0 => 'cn=developers,cn=groups,dc=ad,dc=myuniversity,dc=edu',
      1 => 'cn=sysadmins,cn=groups,dc=ad,dc=myuniversity,dc=edu',
      2 => 'uid=joeprojectmanager,ou=it,dc=ad,dc=myuniversity,dc=edu',
    ),
    ```
- Options B:
  - LDAP group entry attribute holding: `uniquemember`
  - User attribute held in: `dn`



#### LDAP Authorization - Backdrop Roles module <a name="roles"></a>

Implements LDAP authorization for Backdrop roles. It provides an
"authorization consumer" for Backdrop roles.

Dependency:
- LDAP Authorization module

Configuration page:
It has a single configuration entry at admin/config/people/ldap/authorization
That will need to be configured and enabled for authorization to work.
Administration > Configuration > User accounts > LDAP Configuration > Authorization > edit

After configuring an "authorization consumer", use the "test" link to see
the authorizations a given test user would be granted.



#### LDAP Authorization - OG (Organic Groups) module <a name="og"></a>

Implements LDAP authorization for Organic Groups. It provides an
"authorization consumer" for Organic Groups groups.

Dependencies:
- LDAP Authorization module
- Organic groups module

Configuration page:
It has a single configuration entry at admin/config/people/ldap/authorization
That will need to be configured and enabled for authorization to work.
Administration > Configuration > User accounts > LDAP Configuration > Authorization > edit

After configuring an "authorization consumer", use the "test" link to see
the authorizations a given test user would be granted.

##### Use case:

Automate membership and roles in Organic Groups based on LDAP data such as user
attributes or group memberships.
Requirements:
- Configuration that maps Backdrop users to LDAP users (Implemented by
  LDAP Server Module).
- Configuration that maps LDAP user entries to Organic Group membership
  (Implemented by LDAP Authorization - OG (Organic Groups) module).
- LDAP Authorization modules do not require LDAP Authentication to be used.
  LDAP Authorization modules will work with other authentication modules including
  Backdrop authentication. However, there must be a relationship established
  between the Backdrop user and an LDAP entry; this relationship is usually the
  username or email. This relationship is implemented in the LDAP Server module.
  (Some authentication modules in D7: [CAS](https://www.drupal.org/project/cas), [Shibboleth Authentication](https://www.drupal.org/project/shib_auth), [OpenID Connect](https://www.drupal.org/project/openid_connect))
- This takes a little patience to setup and test.

##### Setup

These notes are brief, deferring to more complete instructions are in the
configuration forms.
1. Create the Organic Groups and roles you need. If the default OG roles work
   (member and admin) you do not need to worry about creating roles.
2. Download LDAP project at https://backdropcms.org/project/ldap
3. Enable LDAP Servers and configure an LDAP Server. Only one server can be used
   with LDAP Authorization OG at a time.
4. At admin/config/people/ldap/authorization/add/og_group, create OG Group
   Configuration. After configuring this, a test page will be available.
5. Go to the test page: admin/config/people/ldap/authorization/test/og_group and
   try some usernames to see what OG roles the user would be granted.
6. When you are satisfied with this test with actual users logging in.

##### Tips and Gotchas

- The same configuration options are available in LDAP Authorization Backdrop Roles
  module, but that module is more commonly used. Try configuring LDAP Authorization
  Backdrop Roles if you have trouble with LDAP Authorization OG.
- If you use "group-name" or "role-name" in your mappings instead of numeric ids
  (gid and rid), don't change the names of your groups unless you are going to
  change the mappings at the same time.
- Some helpful logging info can be found by enabling "Detailed LDAP Watchdog
  logging" at admin/config/people/ldap. These logs with be in the watchdog logs.

##### LDAP Authorization OG Storage:

OG authorizations are stored in form gid-rid from the tables `og` (og.gid)
and `og_roles` (og_roles.rid). E.G. 1-2, 2-3, 3-4.
OG in Backdrop does not use machine names so numeric ids are the only way
to store such identifiers.

such as:
````
$user->data = array(
  'ldap_authorizations' => array(
    'og_group' => array (
      '3-2' => array (
        'date_granted' => 1329105152,
      ),
      '2-3' => array (
        'date_granted' => 1329105152,
      ),
    ),
    'backdrop_role' => array (
      '7' => array (
        'date_granted' => 1329105152,
      ),
      '5' => array (
        'date_granted' => 1329105152,
      ),
    ),
  );
````



LDAP Query module <a name="query"></a>
-----------------

A module to allow you to execute custom queries, which can be display in Views
or used in custom solutions. LDAP query builder and storage for queries used by
other ldap modules such as LDAP Feeds module etc.

Dependencies:
- LDAP Servers module

Configuration page:
- Administration > Configuration > User accounts > LDAP Configuration > Queries
  ( admin/config/people/ldap/query )
- Operations:
  - Create query
  - Edit query
  - Disable query
  - Delete query
  - Test query: After configuring a query, use the "test" link to see the results.

You can create and store queries what include these settings:
- Name of query
- LDAP Server used for query
- Base DNs to search in query. e.g. `ou=groups,dc=hogwarts,dc=edu`
- Filter. e.g. (&(objectClass=user) (homePhone=*)) Details:
  [LDAP Filter](https://ldapwiki.com/wiki/LDAP%20Filter%20Choices)
- Attributes to return (e.g. objectclass,name,cn,sn,mail)
- Size Limit of returned data
- Time Limit in seconds
- How aliases should be handled during the search
- Scope of search (Base or Onelevel or Subtree)



LDAP Views module <a name="views"></a>
-----------------

It provides Views integration for LDAP data. It adds new, LDAP-related
filter/sort criteria and fields to those available by views.

Dependencies:
- LDAP Query module
- Views (core module)

Recommended:
- Views UI (core module)

#### How to use

1. Create a new LDAP query: admin/config/people/ldap/query/add
   ( Administration > Configuration > User accounts > LDAP Configuration > Queries > Add LDAP Query )
   It is the data source of the view.
2. Add a new view: admin/structure/views/add
   ( Administration > Structure > Views > Add view )
3. Set "Show: LDAP Query"
4. "Continue & configure" button
5. Other > Query settings > Settings > Set the related LDAP query (step 1.)
6. Set fields: Fields > Add > LDAP ...
7. You can set filter criteria: Filter criteria > Add > LDAP ...
8. You can set sort criteria: Sort criteria > Add > LDAP ...
9. You can set a menu entry. Example:
   - Use "No menu" link.
   - Select "Normal menu entry".
   - Title: title of the menu entry
   - Menu: Primary navigation
   - "Apply" button
10. "Save" button

#### Available LDAP-related fields, filter/sort criteria:

- LDAP Query: CN - Common name
- LDAP Query: DN - Distinguished name
- LDAP Query: Object Class - The LDAP objectClass
- LDAP Query: LDAP Attribute - You can choose an attribute of a query's result.



LDAP Feeds module <a name="feeds"></a>
-----------------

Feeds module integration: Included feeds fetcher for a generic ldap query and
ldap entry parser to turn fetcher data into feeds compatible parser result.
Used to automate content creation based on ldap queries or to automatically
sync users.

Feeds is a general architecture for moving data where an importer consists of
a fetcher, parser, and processor. Ldap Feeds supplies the fetcher and parser
such that any processor can be used (node, user, taxonomy term, etc).

Dependencies:
- LDAP Query module
- Feeds module

Recommended:
- Feeds Admin UI module

#### Example use-cases: <a name="feedsusescases"></a>

- Move course or faculty staff info into Backdrop nodes for directories.
  Rough Recipe: FeedsLdapQueryFetcher for ldap query, FeedsLdapEntryParser
  for parsing it into feeds format, Node Processor for creating/synching nodes.
- Synch LDAP attributes with user profile data.
  Rough Recipe: FeedsBackdropUserLdapEntryFetcher for getting LDAP data,
  FeedsLdapEntryParser for parsing it into feeds format, User Processor for
  creating/synching with Backdrop users.
- Provision Backdrop users with LDAP query.


#### Detailed example 1. <a name="feedsexample1"></a>

Sync LDAP data to existing Backdrop user: step by step example of using
LDAP Feeds "Backdrop User LDAP Entry Fetcher" to bring in profile data of
existing users.

1. Configure an LDAP server. Make sure fields related to LDAP and Backdrop users
   are filled out.
2. Enable the following modules:
   job_scheduler, feeds, feeds_ui, ldap_query, ldap_feeds, field_ui (core module)
3. Add a user account field:
   ( admin/config/people/manage/fields )
     - Surname Field:
       ```
       [Example]
         Label : Surname
         Field type : Text (short)
         Widget : Text field
       [/Example]
       ```
     - Press the "Save" button.
     - Press the "Save field settings" button.
     - Press the "Save settings" button.
4. Create new Feed importer:
   ( admin/structure/feeds/create )
   ```
   [Example]
     Name: LDAP Data to User Fields
     Machine-Readable name: ldap_data_to_user_fields
   [/Example]
   ```
5. Basic settings:
   ( admin/structure/feeds/ldap_data_to_user_fields/settings )
   ```
   [Example]
     Attach to content type : Use standalone form
     Periodic Import : Off (can turn on after testing)
     Import on Submission : Checked
     Processed in background : Unchecked  (Check after testing for larger number of users)
   [/Example]
   ```
6. Set Fetcher to "Backdrop User LDAP Entry Fetcher".
   ( admin/structure/feeds/ldap_data_to_user_fields/fetcher )
7. Fetcher settings:
   ( admin/structure/feeds/ldap_data_to_user_fields/settings/FeedsBackdropUserLdapEntryFetcher )
   ```
   [Example]
     Only return ldap authenticated users : Unchecked
   [/Example]
   ```
8. Set Parser to "LDAP Entry Parser for Feeds".
   ( admin/structure/feeds/ldap_data_to_user_fields/parser )
9. There are no Parser settings.
10. Change processor to "User processor".
    ( admin/structure/feeds/ldap_data_to_user_fields/processor )
11. User processor settings:
    ( admin/structure/feeds/ldap_data_to_user_fields/settings/FeedsUserProcessor )
    ```
    [Example]
      Insert new users : Selected
      Update existing users : Selected
      Text format : Plain text
      Skip non-existent users : Selected
      Status : Active
      Additional roles : Select extra roles to assign to users upon import.
    [/Example]
    ```
12. Set Mappings:
    ( admin/structure/feeds/ldap_data_to_user_fields/mapping )
    SOURCE are fields from LDAP and TARGET are the fields from Backdrop
    user account (e.g. fields of `users` table). If you have selected a test
    user in your ldap server configuration, you should get example values in
    the "legend" sources table.
    ```
    [Example]
      SOURCE      TARGET                   UNIQUE TARGET
      cn          User name (name)         checked
      mail        Email address (mail)     unchecked
      sn          Surname (field_surname)  unchecked
    [/Example]
    ```
12. Log in Backdrop with a new user provided by LDAP. You can see the empty
    "Surname" field on the user account page.
13. Execute the import:
    - Open the page of "LDAP Data to User Fields" importer: import/ldap_data_to_user_fields
      ( URL of the "Feeds importers": base_url/import )
    - "Import" button.
14. You can see the filled "Surname" field on the user account page.

Caveats:
- Feeds User Processor is a little fuzzy on update behavior for users.
- Mapping must include either "name" and "mail" fields as "Unique Targets"
  to affect existing users. GUIDs such as dn used as unique identifiers won't
  affect existing users.
- It should be noted that this process will only sync users that currently exist
  as Backdrop users with their information from an LDAP server. If you want to
  import all of the users from a LDAP server you create an Ldap Query to import
  all of the users: [Detailed example 2.](#feedsexample2)
- There is a section in the "User processor settings" that allows you to delete
  users that are no longer in the feed. This will also delete any nodes that they
  have created. Look for "Action to take when previously imported users are
  missing in the feed".


#### Detailed example 2. <a name="feedsexample2"></a>

Step by step example of using LDAP Feeds "LDAP Query Fetcher" to bring in user
data from LDAP to create new Backdrop users.

1. Configure the LDAP server. Make sure fields related to LDAP and Backdrop users
   are filled out.
2. Enable the following modules:
   job_scheduler, feeds, feeds_ui, ldap_query, ldap_feeds, field_ui (core module)
3. Add user account fields:
   ( admin/config/people/manage/fields )
   ```
   [Example]
     First Name Field :  First Name  field_fname  Text (short)  Text field
     Last Name Field :  Last Name  field_lname  Text (short)  Text field
   [/Example]
   ```
   Include other fields as per your needs further ahead.
4. Create new LDAP Query:
   ( admin/config/people/ldap/query/add )
   ```
   [Example]
     Machine name for this query configuration : ecm_users (Give unique name)
     Name : ECM Users (Human readable name for the query)
     LDAP Server used for query : select your LDAP server
     Enabled : Checked
     Base DNs to search in query : CN=Users,DC=hogwarts,DC=com
     Filter : (&(objectClass=user) (memberOf=CN=give_specific_group_name,CN=Users,DC=hogwarts,DC=com))
     Attributes to return : DN,SN,GIVENNAME,USERPRINCIPALNAME,MAILNICKNAME
   [/Example]
   ```
   - If you want all of users and not of specific group, you can skip section
     `memberOf` completely. Filter then becomes: (objectClass=user)
     Details: [LDAP Filter](https://ldapwiki.com/wiki/LDAP%20Filter%20Choices)
   - Feel free to add your needed attributes.
   - You can test the LDAP query at admin/config/people/ldap/query under
   "OPERATIONS".
5. Create new Feed importer:
   ( admin/structure/feeds/create )
   ```
   [Example]
      Name : LDAP Data to User Data
      Machine-Readable name : ldap_data_to_user_data
   [/Example]
   ``` 
6. Basic settings: 
  ( admin/structure/feeds/ldap_data_to_user_data/settings )
   ```
   [Example]
     Name : LDAP Data to User Data
     Attach to content type : Use standalone form
     Periodic import : 15 min
     Import on Submission : Checked
     Processed in background : Checked
   [/Example]
   ```
   - Uncheck the option "Import on Submission", if you need scheduled
     importation of users from LDAP to Backdrop. (Step 14.)
   - Check the option "Import on Submission", if you want to import users from
     LDAP to Backdrop by manually triggered. (Step 13.)
   - "Save" this Basic settings.
7. Set Fetcher to "LDAP Query Fetcher".
   ( admin/structure/feeds/ldap_data_to_user_data/fetcher )
8. Fetcher Settings:
   ( admin/structure/feeds/ldap_data_to_user_data/settings/FeedsLdapQueryFetcher )
   Select "LDAP Query" in here. In this case: "ECM Users"
   "Save" this Fetcher settings.
9. Set Parser to "LDAP Entry Parser for Feeds".
   ( admin/structure/feeds/ldap_data_to_user_data/parser )
10. Change processor to "User processor".
    ( admin/structure/feeds/ldap_data_to_user_data/processor )
11. User processor settings:
    ( admin/structure/feeds/ldap_data_to_user_data/settings/FeedsUserProcessor )
    ```
    [Example]
      Insert new users : Selected
      Update existing users : Selected
      Text format : Plain Text
      Skip non-existent users : Selected
      Status : Active
      Additional roles : Select extra roles to assign to users upon import.
      Defuse e-mail addresses : Unchecked
    [/Example]
    ```
12. Set Mappings:
    ( admin/structure/feeds/ldap_data_to_user_data/mapping )
    SOURCE are fields from LDAP (result attributes of the "ECM Users" query)
    and TARGET are the fields from Backdrop user account (e.g. fields of
    `users` table).

    ```
    [Example]
      SOURCE              TARGET                      UNIQUE TARGET
      MAILNICKNAME        User name (name)            checked
      USERPRINCIPALNAME   Email address (mail)        checked
      GIVENNAME           First Name (field_fname)    unchecked
      SN                  Last Name (field_lname)     unchecked
    [/Example]
    ```
13. Manually triggered import:
    - Open the page of "LDAP Data to User Data" importer: import/ldap_data_to_user_data
      ( URL of the "Feeds importers": base_url/import )
    - Select the related query: "ECM Users" (step 4.)
    - "Import" button.
14. Lets see how can we schedule this importer to execute periodically to
    import users from LDAP to Backdrop:

Assuming we have created a module named "import_data".

A. Implement hook_cronapi()
  ```
  function import_data_cronapi($op, $job = NULL) {
    return array(
      'import_data_cronjob_1' => array(
        'title' => 'Import LDAP Users',
        'callback' => 'import_data_ldap_users_callback',
        'enabled' => TRUE,
        'scheduler' => array(
          'name' => 'crontab',
          'crontab' => array(
            'rules' => array('0+@ */12 * * *'), // Schedule for import once in 12 hours
          ),
        ),
      ),
    );
  }
  ```

B. Write Function to actually import
  ```
  function import_data_ldap_users_callback($job) {
    $vars = array();
    if (function_exists('feeds_source')){
      while (FEEDS_BATCH_COMPLETE != feeds_source('ldap_data_to_user_data')->import());
      watchdog('Cron LDAP Users Import', t('LDAP Users Imported Successfully.'), $vars, WATCHDOG_INFO,NULL);
    }
    else {
      watchdog('Cron LDAP Users Import', t('Function : feeds_source not found.'), $vars, WATCHDOG_ERROR,NULL);
    }
  }
  ```

**Done! You have successfully scheduled a cron to import LDAP Users into Backdrop**

Caveats:
- Feeds User Processor is a little fuzzy on update behavior for users.
- Mapping must include either "name" and "mail" fields as "Unique Targets"
  to affect existing users. GUIDs such as dn used as unique identifiers won't
  affect existing users.



LDAP Test module <a name="test"></a>
----------------

Module for testing the LDAP module. Only for development and debugging purposes.
It's required by LDAP Help module.

#### Summary of simpletest framework for LDAP_* modules

Configuration sources for LDAP Simpletests:
- ldap_test/module_name.conf.inc (e.g. ldap_servers.conf.inc) contain functions
  such as `ldap_test_ldap_servers_data()` that return arrays of configuration data
  keyed a test id.
- ldap_test/test_ldap/ldap_data_id (e.g. ldap_test/test_ldap/hogwarts) contain
  the data used to populate the LDAP. The stucture of the actual LDAP array
  depends on which server configuration if driving it. For example if the LDAP
  server configuration has a `memberof` attribute, the `memberof` attribute will be
  populated in the users.

Related sources:
- https://trac-hacks.org/wiki/LdapPluginTests
- https://en.wikipedia.org/wiki/Hogwarts
- https://harrypotter.wikia.com/wiki/Hogwarts_School_of_Witchcraft_and_Wizardry



LDAP Help module <a name="help"></a>
----------------

It is just for debugging, administrator help and reporting issues. Use it if
you have problems. Disable it in production. It adds no functionality or end
user help.

Dependencies:
- LDAP Servers module
- LDAP Test module

Configuration page:
- Administration > Configuration > User accounts > LDAP Configuration > Help
  ( admin/config/people/ldap/help )
- LDAP Module Resources
- Status page
- LDAP Watchdog errors and notifications
- How to report bugs in LDAP project
- Sample LDAPs:
  - Active Directory
  - OpenLDAP



LDAP SSO module <a name="sso"></a>
---------------

Provides Kerberos/NTLM single-sign-on.
**This module is now a [separate project on Backdropcms.org](https://www.backdropcms.org/project/ldap_sso)**
But the configure page is integrated into the LDAP Authentication module.

Configuration page:
- Administration > Configuration > User accounts > LDAP Configuration > Authentication
  ( admin/config/people/ldap/authentication )
- Cookie lifetime
- Authentication mechanism
- SSO excluded paths
- SSO excluded hosts



Tokens <a name="tokens"></a>
------
Some fields in the LDAP modules allow for "ldap tokens". For example:
Administration > Configuration > User accounts > LDAP Configuration > User > LDAP Servers Providing Provisioning Data: Use server
These tokens are replaced by values within an LDAP entry retrieved from the
PHP LDAP extension.

#### Example Use Cases
The following are based on the LDAP entry below.

| Use Case                  | Template              | Evaluates to          |
| ------------------------- |-----------------------|-----------------------|
| ldap server mail template |`[cn]@myuniversity.edu`|`jdoe@myuniversity.edu`|
| ldap server mail template |`[cn]@[dc:1].edu`      |`jdoe@myuniversity.edu`|

#### Example Illustrating Derivation of Token Values
An LDAP entry array such as:
```
'dn' => 'cn=jdoe,ou=campus accounts,ou=toledo campus,dc=ad,dc=myuniveristy,dc=edu',
'mail' => array( 0 => 'jdoe@myuniversity.edu', 'count' => 1),
'sAMAccountName' => array( 0 => 'jdoe', 'count' => 1),
```
Would have the following tokens available for its templates:
- from dn attribute:
  ```
    [cn] = jdoe
    [cn:0] = jdoe
    [cn:last] => jdoe
    [ou] = campus accounts
    [ou:0] = campus accounts
    [ou:1] = toledo campus
    [ou:last] = toledo campus
    [dc] = ad
    [dc:0] = ad
    [dc:1] = myuniveristy
    [dc:2] = edu
    [dc:last] = edu
  ```
- derived from other attributes:
  ```
    [mail] = jdoe@myuniversity.edu
    [mail:0] = jdoe@myuniversity.edu
    [mail:last] = jdoe@myuniversity.edu
    [samaccountname] = jdoe
    [samaccountname:0] = jdoe
    [samaccountname:last] = jdoe
  ```

#### Additional Example Tokens
Use the test link at the servers page: admin/config/people/ldap/servers
You can see the tokens and sample values when you enter a test username and
submit the form.



Automated testing <a name="automatedtesting"></a>
-----------------

You can test four LDAP modules by Simpletest:
1. Enable the Simpletest module:
   Administration > Functionality > List modules > Testing: Enabled
2. Open the Testing page: Administration > Configuration > Development > Testing
3. Select these tests:
   - LDAP Servers Tests
   - LDAP User Integration Tests
   - LDAP User Unit Tests
   - LDAP User User Interface
   - LDAP Authentication Tests
   - LDAP Authorization Basic Tests
   - OG 1.x-2.x Tests. (This requires the OG module. /Organic groups/)
4. Press "Run tests" button.



Manual testing <a name="manualtesting"></a>
--------------
(Detailed examples: [Local LDAP test server by Vagrant](#vagranttesting))

#### LDAP Server module

After configuring a server, use the "test" link.
Available at admin/config/people/ldap/servers
Fill the form, and press the "Test" button.



#### LDAP User module

##### LDAP User test form

A test page is available at admin/config/people/ldap/user/test
for testing your LDAP User configuration. The value of this form is to see
what would happen based on your current configuration or to actually execute
an action for a single account. This can be very useful to confirm your
LDAP user and LDAP server configuration.

To use, enter a test Backdrop username and check the events you want to test.
The resulting page will show what the provisioning would be. If you select
the "Execute Action" mode, the transactions configured will be performed
(for that user).

Notes about the resulting arrays:
- Devel module must be enabled for this to work. https://backdropcms.org/project/devel
- In provisioning or synching to LDAP
  (e.g. provisionLdapEntryResults => context => Update Backdrop User Synch Context => proposed)
  only the dn and attributes that will be provisioned or modified are visible.
  These are in the "proposed" array.
- In provisioning or synching to Backdrop, only the values that will be passed
  into the $user_edit array in user_save($account, $user_edit) are shown.
  These are in the "proposed" array.


##### LDAP User test scripts

Manual testing scripts are available at: ldap_user/tests/ldap_user.test.manual.txt
These are handy for understanding the expected behavior of the ldap user module.



#### LDAP Authentication

Login with a user provided by LDAP.



#### LDAP Authorization module

After configuring an "authorization consumer", use the "test" link to see
the authorizations a given test user would be granted.
Available at admin/config/people/ldap/authorization
Fill the form, and press the "Test" button. The result page shows detailed
authorization data.



#### LDAP Query module

After configuring a query, use the "test" link to see the result of the query.
Available at admin/config/people/ldap/query



#### LDAP Views module

Create a new view: [LDAP Views module](#views) > How to use



#### LDAP Feeds

Try these:
- [Detailed example 1.](#feedsexample1)
- [Detailed example 2.](#feedsexample2)



Local LDAP test server by Vagrant <a name="vagranttesting"></a>
---------------------------------
You can test the LDAP module with a local LDAP test server.
Main steps:
1. [Building a test environment](#buildingvagranttesting)
2. [Configure the LDAP module for the local test server](#configvagranttesting)
3. [Manual tests of the LDAP module on the local test server](#manualvagranttesting)

#### 1. Building a test environment <a name="buildingvagranttesting"></a>

You can build a test environment with this description. You can download and
install a prepared configuration. There is a `Vagrantfile` included that will
build a virtual machine with a working LDAP directory.

1. Install VirtualBox:
   - https://www.virtualbox.org/
     (Version 6.1.10 tested.)
   - Enable the virtualization in the BIOS.
   - The language of Virtualbox must be "English", because Vagrant reads
     VirtualBox's responses.
2. Install Vagrant: https://www.vagrantup.com/
   (Version 2.2.9 tested.)
3. Download this project: https://github.com/VasasA/simple_ldapVM/archive/7.x-1.x.zip
(It is a fork of https://github.com/ulsdevteam/simple_ldap)
4. Unzip it into a directory.
5. Open Terminal, and `cd` to this directory (containing the `Vagrantfile`).
6. Run this command: `vagrant up`
   It will download and build a virtual machine with a working LDAP directory.
   (It may take a long time.)
7. When complete, there is the IP address in the last line. If OS X is the
   Vagrant host, then the vagrant box is available at `simpleldap.local`
   For other operating systems, the IP address will need to be obtainted manually,
   and added to the local hosts file for best results. (%WinDir%\System32\drivers\etc)
8. You can create some new LDAP users:
   - Open phpLDAPadmin. Available at http://simpleldap.local/pma
     - Login DN: cn=admin,dc=local
     - password: admin
   - Select the `ou=people` in the tree.
   - Use the "Create a child entry" link.
   - Select "Default".
   - ObjectClass: inetOrgPerson
   - Press the "Proceed" button.
   - Create Object:
     - RDN: cn
     - cn: "username"
     - sn: "surname"
     - Email: "user email address"
     - Password: "user password"
     - Press the "Create Object" button.
   - Press the "Commit" button.
9. You can create some new LDAP groups:
   - Open phpLDAPadmin. Available at http://simpleldap.local/pma
     - Login DN: cn=admin,dc=local
     - password: admin
   - Select the `ou=groups` in the tree.
   - Use the "Create a child entry" link.
   - Select "Default".
   - ObjectClass: groupOfNames
   - Press the "Proceed" button.
   - Create Object:
     - RDN: cn
     - cn: "name of the group"
     - member: You must set up at least one user. Example: cn=ldapuser,ou=people,dc=local
     - Press the "Create Object" button.
   - Press the "Commit" button.
10. After testing, you can shut down the virtual machine with this command:
    `vagrant halt`

**LDAP**
- The LDAP is pre-populated with some dummy data. Available at:
ldap://simpleldap.local
- DN: cn=admin,dc=local
- password: admin

Or:
- DN: cn=ldapuser,ou=people,dc=local
- password: ldapuser

**phpLDAPadmin**
- phpLDAPadmin is available at http://simpleldap.local/pma
- Login DN: cn=admin,dc=local
- password: admin

**Virtual machine's console or ssh credentials**
- username: vagrant
- password: vagrant

**Drupal 7**
- The virtual machine also contains a Drupal 7 installation with Simple LDAP module.
- Available at: http://simpleldap.local/
- username: admin
- password: admin


#### 2. Configure the LDAP module for the local test server <a name="configvagranttesting"></a>

Configuration page: Administration > Configuration > User accounts > LDAP Configuration

- Settings: admin/config/people/ldap
    ```
    [Example]
      Obfuscate LDAP Passwords : Clear text
    [/Example]
    ```
- Add LDAP Server Configuration: admin/config/people/ldap/servers/add
    ```
    [Example]
      Machine name for this server configuration. : local
      Name : local
      Enabled : Checked
      LDAP Server Type : Open LDAP
      LDAP server : simpleldap.local
      LDAP port : 389
      Use Start-TLS : Unchecked
      Binding Method for Searches : Service Account Bind
      DN for non-anonymous search : cn=admin,dc=local
      Password for non-anonymous search : admin
      Clear existing password from database : Unchecked
      Base DNs for LDAP users, groups, and other entries : dc=local
      AuthName attribute : cn
      AccountName attribute : cn
      Email attribute : mail
      Testing Backdrop Username : ldapuser
      DN of testing username : cn=ldapuser,ou=people,dc=local
      Name of Group Object Class : groupOfNames
      LDAP Group Entry Attribute Holding User's DN, CN, etc. : member
      User attribute held in "LDAP Group Entry Attribute Holding..." : dn
      Testing LDAP Group DN : cn=test_group,ou=groups,dc=local
      Testing LDAP Group DN that is writable : cn=test_group,ou=groups,dc=local
    [/Example]
    ```

- User: admin/config/people/ldap/user
    ```
    [Example]
      How to resolve LDAP conflicts with manually created Backdrop accounts : Reject manual creation of Backdrop accounts that conflict with LDAP Accounts.
      LDAP Servers Providing Provisioning Data : None
      LDAP Servers to Provision LDAP Entries on : None
    [/Example]
    ```

- Authentication: admin/config/people/ldap/authentication
    ```
    [Example]
      Allowable Authentications : Mixed mode.
      local (simpleldap.local) : Checked
      Email Behavior : Show disabled email field on user forms with LDAP derived email.
      Email Update : Update stored email if LDAP email differs at login but don't notify user.
      Email Template Handling : Never use the template.
      Password Behavior : Display password field disabled
    [/Example]
    ```

- Add the "Backdrop role" consumer: admin/config/people/ldap/authorization/add/backdrop_role
    ```
    [Example]
      local : Selected
      Enable this configuration : Checked
      Only apply the following LDAP to Backdrop role configuration to users authenticated via LDAP : Checked
      Convert full dn to value of first attribute before mapping : Checked
      Only grant Backdrop roles that match a filter above: Unchecked
      When a user logs on : Checked
      Revoke Backdrop roles previously granted by LDAP Authorization but no longer valid : Checked
      Re grant Backdrop roles previously granted by LDAP Authorization but removed manually : Checked
      Create Backdrop roles if they do not exist : Checked
    [/Example]
    ```

- Add LDAP Query: admin/config/people/ldap/query/add
    ```
    [Example]
      Machine name for this query configuration: users
      Name : users
      local : Selected
      Enabled : Checked
      Base DNs to search in query : ou=people,dc=local
      Filter : (sn=*)
      Attributes to return : sn,objectClass,mail,cn,dn
      Size Limit of returned data : 0
      Time Limit in Seconds : 0
      How aliases should be handled during the search : (default) aliases are never dereferenced.
      Scope of search : SUBTREE. (default)
    [/Example]
    ```

#### 3. Manual tests of the LDAP module on the local test server <a name="manualvagranttesting"></a>

- LDAP Servers:
  - Use the "test" link available at admin/config/people/ldap/servers
  - The form is filled, but you have to enable this option:
    "LDAP Group Entry Attribute Holding User's DN, CN is a required attribute of the group."
  - Press the "Test" button.


- LDAP User:
  - Devel module must be enabled. https://backdropcms.org/project/devel
  - Test page available at admin/config/people/ldap/user/test
  - Enter a test Backdrop username and check the events you want to test.
  - Press the "Test" button.
  - More details: [Manual testing](#manualtesting) > LDAP User module > LDAP User test form


- LDAP Authentication: Login with a user provided by LDAP. If you want to create a
    new LDAP user: [Building a test environment](#buildingvagranttesting) > Step 8.


- LDAP Authorization:
  - Login with a user provided by LDAP. If you want to create a new LDAP user:
    [Building a test environment](#buildingvagranttesting) > Step 8.
  - Verify that the user has appeared in the Backdrop user accounts list: admin/people
  - Create a new LDAP group: [Building a test environment](#buildingvagranttesting) > Step 9.
    Let the LDAP user be a member of this new LDAP group.
  - Use the "test" link available at admin/config/people/ldap/authorization
  - Write the username in this field:
    "Backdrop usernames to test role authorizations results for"
  - Press the "Test" button.
  - The result page shows the name of LDAP group under "Authorization IDs".


- LDAP Query:
  - A query was created in the previous section: Add LDAP Query
    Name of the query: users
  - Use the "test" link of this query. Available at admin/config/people/ldap/query
  - The result page shows the users provided by the LDAP.


- LDAP Views:
  1. A query was created in the previous section: Add LDAP Query
     It is the data source of the view. Name of the query: users
  2. Add a new view: admin/structure/views/add
     ( Administration > Structure > Views > Add view )
     - View name: User list
     - Show: LDAP Query
     - Press the "Continue & configure" button.
  3. Selet the query of the view:
     - Use "Settings" link: Other > Query settings > Settings
       ( admin/structure/views/nojs/display/user_list/page/query )
     - LDAP Search: users
     - Press the "Apply" button.
  4. Set a field: 
     - Fields > Add
     - Select "LDAP Query: DN".
     - Press the "Add and configure fields" button.
     - Press the "Apply (all displays)" button.
  5. Set another field: 
     - Fields > Add
     - Select "LDAP Query: LDAP Attribute".
     - Press the "Add and configure fields" button.
     - Label: Email address
     - Values to show: All values
     - Attribute name: mail
     - Press the "Apply (all displays)" button.
  6. Set sort criteria: 
     - Sort criteria > Add
     - Select: "LDAP Query: DN"
     - Press the "Add and configure sort criteria" button.
     - Press the "Apply (all displays)" button.
  7. You can set a menu entry:
     - Use "No menu" link: Page settings > Menu > No menu
     - Select "Normal menu entry"
     - Title: LDAP users
     - Menu: Primary navigation
     - Press the "Apply" button.
  8. Press the "Save" button.
  9. Use the "LDAP users" menu on the homepage.


- LDAP Feeds: Sync LDAP data to existing Backdrop user:
  1. Enable the following modules:
     job_scheduler, feeds, feeds_ui, ldap_query, ldap_feeds, field_ui (core module)
  2. Add a user account field: 
     - admin/config/people/manage/fields
     - Add new field:
       ```
       [Example]
         Label : Surname
         Field type : Text (short)
         Widget : Text field
       [/Example]
       ```
     - Press the "Save" button.
     - Press the "Save field settings" button.
     - Press the "Save settings" button.
     - You can see the new empty "Surname" field on a user account page.
       ( user/1/edit )
  3. Create new Feed importer:
     - admin/structure/feeds/create
       ```
       [Example]
         Name : LDAP Data to User Fields
         Machine-Readable name : ldap_data_to_user_fields
       [/Example]
       ```
     - Press the "Create" button.
  4. Basic settings: 
     - admin/structure/feeds/ldap_data_to_user_fields/settings
       ```
       [Example]
         Attach to content type : Use standalone form
         Periodic Import : Off
         Import on Submission : Checked
         Processed in background : Unchecked
       [/Example]
       ```
     - Press the "Save" button. 
  5. Select a fetcher:
     - admin/structure/feeds/ldap_data_to_user_fields/fetcher
       ```
       [Example]
         Backdrop User LDAP Entry Fetcher : Selected
       [/Example]
       ```
     - Press the "Save" button.
  6. Fetcher settings:
     - admin/structure/feeds/ldap_data_to_user_fields/settings/FeedsBackdropUserLdapEntryFetcher
       ```
       [Example]
         Only return ldap authenticated users : Unchecked
       [/Example]
       ```
     - Press the "Save" button.
  7. Select a parser:
     - admin/structure/feeds/ldap_data_to_user_fields/parser
       ```
       [Example]
         LDAP Entry Parser for Feeds : Selected
       [/Example]
       ```
     - Press the "Save" button.
  8. There are no Parser settings.
  9. Select a processor:
     - admin/structure/feeds/ldap_data_to_user_fields/processor
       ```
       [Example]
         User processor : Selected
       [/Example]
       ```
     - Press the "Save" button.
  10. User processor settings:
      - admin/structure/feeds/ldap_data_to_user_fields/settings/FeedsUserProcessor
        ```
        [Example]
          Insert new users : Selected
          Update existing users : Selected
          Text format : Plain text
          Skip non-existent users : Selected
          Status : Active
          Additional roles : (none)
        [/Example]
        ```
      - Press the "Save" button.
  11. Set Mappings:
      - admin/structure/feeds/ldap_data_to_user_fields/mapping
        SOURCE are fields from LDAP and TARGET are the fields from Backdrop
        user account (e.g. fields of `users` table). If you have selected a
        test user in your LDAP server configuration, you should get example
        values in the "legend" sources table.
      - User name field:
        ```
        [Example]
          SOURCE : cn
          TARGET : User name (name)
        [/Example]
        ```
        - Press the "Save" button.
        - Press the "Gear" button under "Target configuration" and select "Unique".
        - Press the "Update" button.
        - Press the "Save" button.
      - Email address field:
        ```
        [Example]
          SOURCE : mail
          TARGET : Email address (mail)
        [/Example]
        ```
        - Press the "Save" button.
      - Surname field:
        ```
        [Example]
          SOURCE : sn
          TARGET : Surname (field_surname)
        [/Example]
        ```
        - Press the "Save" button.
  12. Log in Backdrop with a new user provided by LDAP.
      - For example:
        - Username: ldapuser
        - Password: ldapuser
      - You can see the empty "Surname" field on the user account page.
  13. Execute the import:
      - Open the page of "LDAP Data to User Fields" importer:
        import/ldap_data_to_user_fields
        ( URL of the "Feeds importers": base_url/import )
      - Press the "Import" button.
  14. You can see the filled "Surname" field on the user account page.


- LDAP Feeds: Using LDAP Feeds "LDAP Query Fetcher" to bring in user data from
  LDAP to create new Backdrop users:
  1. Enable the following modules:
     job_scheduler, feeds, feeds_ui, ldap_query, ldap_feeds
  2. Create new LDAP Query:
     - admin/config/people/ldap/query/add
       ```
       [Example]
         Machine name for this query configuration : get_ldap_users
         Name : Get LDAP Users
         local : Selected (LDAP Server used for query.)
         Enabled : Checked
         Base DNs to search in query : ou=people,dc=local
         Filter : (objectClass=inetOrgPerson)
         Attributes to return : cn,mail
       [/Example]
       ```
     - Press the "Add" button.
       You can test the LDAP query at admin/config/people/ldap/query under
       "OPERATIONS".
  3. Create new Feed importer: admin/structure/feeds/create
     ```
     [Example]
        Name : LDAP Data to User Data
        Machine-Readable name : ldap_data_to_user_data
     [/Example]
     ``` 
     - Press the "Create" button.
  4. Basic settings:
     - admin/structure/feeds/ldap_data_to_user_data/settings
     ```
     [Example]
       Attach to content type : Use standalone form
       Periodic import : Off
       Import on Submission : Checked
       Processed in background : Unchecked
     [/Example]
     ```
     - Press the "Save" button.
  5. Select a Fetcher:
     - admin/structure/feeds/ldap_data_to_user_data/fetcher
       ```
       [Example]
         LDAP Query Fetcher : Selected
       [/Example]
       ```
     - Press the "Save" button.
  6. Fetcher settings: 
     - admin/structure/feeds/ldap_data_to_user_data/settings/FeedsLdapQueryFetcher
     Select "LDAP Query" in here. In this case, "Get LDAP Users"
       ```
       [Example]
         LDAP Query : Get LDAP Users
       [/Example]
       ```
     - Press the "Save" button.
  7. Select a parser:
     - admin/structure/feeds/ldap_data_to_user_data/parser
       ```
       [Example]
         LDAP Entry Parser for Feeds : Selected
       [/Example]
       ```
     - Press the "Save" button.
  8. There are no Parser settings.
  9. Select a processor:
     - admin/structure/feeds/ldap_data_to_user_data/processor
       ```
       [Example]
         User processor : Selected
       [/Example]
       ```
     - Press the "Save" button.
  10. User processor settings:
      - admin/structure/feeds/ldap_data_to_user_data/settings/FeedsUserProcessor
        ```
        [Example]
          Insert new users : Selected
          Update existing users : Selected
          Text format : Plain text
          Skip non-existent users : Selected
          Status : Active
          Additional roles : (none)
          Defuse e-mail addresses : Unchecked
        [/Example]
        ```
      - Press the "Save" button.
  11. Set Mappings:
      - admin/structure/feeds/ldap_data_to_user_data/mapping
        SOURCE are fields from LDAP (result attributes of the "Get LDAP Users"
        query) and TARGET are the fields from Backdrop user account (e.g.
        fields of `users` table).
      - User name field:
        ```
        [Example]
          SOURCE : cn
          TARGET : User name (name)
        [/Example]
        ```
        - Press the "Save" button.
        - Press the "Gear" button under "Target configuration" and select "Unique".
        - Press the "Update" button.
        - Press the "Save" button.
      - Email address field:
        ```
        [Example]
          SOURCE : mail
          TARGET : Email address (mail)
        [/Example]
        ```
      - Press the "Save" button.
  12. Manually triggered import:
      - Open the page of "LDAP Data to User Data" importer:
        import/ldap_data_to_user_data
        ( URL of the "Feeds importers": base_url/import )
      - LDAP Query : Get LDAP Users
      - Press the "Import" button.
  13. You can see the imported users on the user accounts page:
      admin/people



LDAP test script without Backdrop<a name="testscript"></a>
---------------------------------

Location of the script file: `ldap_help/ldap_test_script`
This script is intended to help separate LDAP module configuration and bugs
from LDAP server, ldap php extension, and related connectivity and LDAP
permissions issues. It uses the php ldap extension functions like
ldap_connect(), ldap_search(), etc. rather than the Backdrop LDAP module code.
The test script does not depend on the Backdrop LDAP module and should not be
run within a web server context.
This script is preconfigured by `config.inc` according to the "[Local LDAP test server by Vagrant](#vagranttesting)".
Details: ldap_help/ldap_test_script/README.md



Debugging <a name="debugging"></a>
---------

1. Enable LDAP Help module (ldap_help) and detailed watchdog logging to get more
   information on what is occurring:
   - Use LDAP Help module status page: admin/config/people/ldap/help/status
   - Enable "Detailed LDAP Watchdog logging": admin/config/people/ldap
   - Enable Database Logging module: admin/modules/list
2. Try to understand the responses in the log and the different stages in
   which they occur: admin/reports/dblog
3. Try narrowing down your problem by making sure that the each step works
   before moving on to the next. (Use "[Manual testing](#manualtesting)")
   i.e.:
     - Connection
     - Bind
     - Search
     - Authentication
     - Authorization
4. Isolating LDAP problems from LDAP Module problems
   When things get tricky, sometimes its best to go to the php scripts or
   command line queries of ldap to make sure a problem is in Backdrop or
   LDAP modules and not in the LDAP server, PHP ldap extension, or particular
   LDAP user or group entries:
   - For authentication issues, make sure the user is able to use other
     software with their LDAP credentials.
   - Try a PHP script that is not tied to Backdrop such as the one in the
     LDAP Help module within ldap_help/ldap_test_script:
     [LDAP test script without Backdrop](#testscript)
     This will help isolate Backdrop issues from php ldap extension and
     LDAP server problems.
   - Use another tool such as apache directory or command line LDAP queries.
     For example: 
     `$ ldapsearch -H ldap://127.0.0.1 -x -b dc=people,dc=local -D joe@example.local -w 'Thepassword'`
5. Debugging LDAP Authorization
   - Go to the test form: admin/config/people/ldap/authorization/test/backdrop_role
   - Submit with a username.
   - In the response page to the form, examine the "Prefiltered and Final Mappings"
     section. It lists all the "raw authorizations" or the authorizations before
     filtering and mapping. If you do not see the raw authorizations you expect,
     your "II. LDAP to Backdrop role mapping and filtering" is off:
     admin/config/people/ldap/authorization/edit/backdrop_role
     Otherwise your problem is likely in part III. of the authorization configuration
     which triggers when authorization happens.
   - There is some ability to see intermediary data in the authorization code.
     This can be helpful for debugging. Enable "Detailed LDAP Watchdog logging"
     as above. Then log on as the user in question. There will be detailed logs
     in watchdog of the LDAP authorization steps: admin/reports/dblog
6. Picking data
   Picking through the database and the configuration can be helpful. Here are some
   queries. The serialized data can be better viewed at http://unserialize.net/serialize.
   - table: `ldap_authmap`: Will contain a record for every user who is ldap authenticated.
     `select * from ldap_authmap where module like 'ldap%';`
   - table: `users`: LDAP data specific to a user will be stored in the `data` field of this table.
     `select cast(data as char(1000)) from users where data like '%ldap%';`
   - table: `ldap_servers`
   - table: `ldap_authorization`: Data used to map users LDAP entry to authorization rights.
   - Picking through configuration:
     - admin/config/development/configuration/single/export
     - Configuration group: Configuration
     - Configuration name: Select an LDAP module.
     - Copy the content of "Exported configuration" field.



#### Common Error Messages and Warnings

- "No LDAP Extension is loaded for PHP"
  Signifies the php LDAP extension is not enabled. Use the same steps you would 
  to install any php extension. If it is available with the PHP version you
  installed, it is simply a matter of uncommenting the LDAP link in the php.ini
  file. e.g. `extension=php_ldap.dll` for Windows.
- "Possible invalid format for ... The format may be correct for your ldap,
  but please double check."
  This warning is given when an LDAP attribute name (cn, dn, mail, etc.) is
  checked to be within LDAP standards. Active Directory and other LDAP
  implementations commonly break these standards, so your attribute name may
  be correct and still get this error.
- "Could Not start TLS errors. Could not start TLS. (Error -11: Connect error).
   Warning: ldap_start_tls() [function.ldap-start-tls]: Unable to start TLS: Connect error in ..."
   These are commonly certificate or openLDAP configuration problems.
   Here are some debugging steps:
   - Try connecting with start TLS turned off. If this succeeds it shows you have
     the right server connection. If it fails it doesn't tell you anything new.
   - Try connecting from php command line with the test script in
     [#1292786: LDAP Server: Server test startTLS fails connecting to 389-DS](https://www.drupal.org/project/ldap/issues/1292786)
     to take Backdrop, Apache, and LDAP modules out of the possible causes.
     Use "[LDAP test script without Backdrop](#testscript)".
   - Make sure openLDAP is configured correctly. 
     See Client Configuration section of http://www.openldap.org/doc/admin24/tls.html
     and http://www.openldap.org/faq/data/cache/185.html
   - Make sure the LDAP server accepts connections from your server.
     If you have access to the LDAP server logs, they may be useful.

#### Need to use LDAPS
Some LDAP servers require ldaps. Make sure to do the following:
- Enable open_ssl php extension.
- Use `ldaps://myldapserver.com` format for LDAP server.
- On a windows server, see [How to enable LDAP over SSL with a third-party certificate authority](http://support.microsoft.com/kb/321051).

#### Need to use Start TLS
To use TLS, you either need your certificate to work OR need to configure LDAP
to never require a certificate.



Issues <a name="issues"></a>
------

Bugs and Feature requests should be reported in the Issue Queue:
https://github.com/backdrop-contrib/ldap/issues
Please always provide detailed log output and information on your configuration.
See also admin/config/people/ldap/help/issues for debugging output you can copy
and paste into the issue.


Current Maintainer <a name="maintainer"></a>
------------------

- Attila Vasas (https://github.com/vasasa).
- Seeking additional maintainers.


Credits <a name="credits"></a>
-------

- Ported to Backdrop CMS by Attila Vasas (https://github.com/vasasa).
- Originally written for Drupal: https://www.drupal.org/node/806060/committers


License <a name="license"></a>
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.


Screenshot <a name="screenshot"></a>
----------

![Simple LDAP screenshot](https://github.com/backdrop-contrib/ldap/blob/1.x-2.x/images/screenshot.png)

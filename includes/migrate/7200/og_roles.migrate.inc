<?php

/**
 * @file
 * Add per-bundle OG roles.
 *
 * Class should be included only if this is an upgrade from branch 7.x-1.x
 * to branch 7.x-2.x
 */


if (db_field_exists('og_membership', 'group_type') && db_table_exists('og') && !db_table_exists('d6_og')) {

class OgMigrateRoles extends OgEntityMigration {

  public $tableName = 'og_role';

  public $keyName = 'rid';

  protected $dependencies = array('OgMigrateMembership');

  /**
   * Indicate we are updating existing data.
   */
  protected $systemOfRecord = Migration::DESTINATION;

  public function __construct() {
    $this->description = t('Add per-bundle OG roles.');

    $this->query = db_select('og_role', 'ogr')
      ->fields('ogr', array('rid', 'gid', 'group_bundle'))
      ->condition('gid', 0, '>')
      ->condition('group_type', '', '=')
      ->condition('group_bundle', '', '=')
      ->orderBy('rid');

    parent::__construct();

    $this->addFieldMapping('rid', 'rid');
    $this->addFieldMapping('gid', 'gid');
    $this->addFieldMapping('group_type', 'group_type');
    $this->addFieldMapping('group_bundle', 'group_bundle');

    // Copy all existing global roles to bundle-specific versions.
    // Although similar processing is available through the
    // og_roles_override() function, special handling is necessary to
    // ensure that custom global roles are copied as well as default
    // global roles.

    // This call to og_roles searches the database for all roles where
    // bundle and type are blank and gid = 0. Such entries should only
    // exist when a pre-2.0 version of og has not been fully migrated.
    $og_roles = og_roles('', '', 0);
    if (!empty($og_roles)) {
      $perms = og_role_permissions($og_roles);
    }
    else {
      // Just to be safe, revert to standard list of global default roles
      // if no matches were found.
      $og_roles = og_get_default_roles();
      $perms = og_get_default_permissions();
    }

    foreach (og_get_all_group_bundle() as $group_type => $bundles) {
      foreach ($bundles as $bundle => $label) {
        // Skip processing if already done.
        // Note that og_roles->__construct can be called multiple times
        // during a single migration.
        if (og_roles($group_type, $bundle, 0, TRUE)) {
          continue;
        }
        foreach ($og_roles as $rid => $name) {
          // Copy each role and its permissions to each bundle.
          // Although og_roles_override() does a db query at this point to
          // remap og_user_roles, not necessary in this case
          // (handled by og_user_roles.migrate).
          $role = og_role_create($name, $group_type, 0, $bundle);
          og_role_save($role);
          og_role_change_permissions($role->rid, $perms[$rid]);
        }
      }
    }
  }

  public function prepareRow($row) {
    list($group_type, $gid) = $this->getNewGroupId($row->gid);
    // Some installations might have missing entities, so we don't assume
    // they exist.
    if (!$gid || !$group = entity_load_single($group_type, $gid)) {
      return FALSE;
    }
    list(,,$bundle) = entity_extract_ids($group_type, $group);

    $row->group_type = $group_type;
    $row->gid = $gid;
    $row->group_bundle = $bundle;
    return parent::prepareRow($row);
  }
}

}
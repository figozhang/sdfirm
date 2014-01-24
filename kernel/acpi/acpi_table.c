#include <target/acpi.h>

#define ACPI_TABLE_LIST_INCREMENT	4
#define ACPI_TABLE_LIST_FIXED		2

#define foreach_table_handle_garbage(ddb)			\
	for (ddb = ACPI_TABLE_LIST_FIXED;			\
	     ddb < acpi_gbl_table_list.use_table_count; ddb++)	\
		if (acpi_gbl_table_list.tables[ddb].flags & ACPI_TABLE_IS_GARBAGE)
#define foreach_table_handle(ddb, start)			\
	for (ddb = start;					\
	     ddb < acpi_gbl_table_list.use_table_count; ddb++)	\
		if (!(acpi_gbl_table_list.tables[ddb].flags & ACPI_TABLE_IS_GARBAGE))

static struct acpi_table_list acpi_gbl_table_list;
static boolean 	acpi_gbl_table_finalizing = false;
acpi_mutex_t acpi_gbl_table_mutex;
uint8_t acpi_gbl_integer_bit_width = 64;

static void acpi_table_lock(void)
{
	(void)acpi_os_acquire_mutex(&acpi_gbl_table_mutex, ACPI_WAIT_FOREVER);
}

static void acpi_table_unlock(void)
{
	acpi_os_release_mutex(&acpi_gbl_table_mutex);
}

static void acpi_table_notify(struct acpi_table_desc *table_desc,
			      acpi_ddb_t ddb, uint32_t event)
{
	acpi_table_increment(ddb);
	acpi_table_unlock();
	acpi_event_table_notify(table_desc, ddb, event);
	acpi_table_lock();
	acpi_table_decrement(ddb);
}

boolean acpi_table_contains_aml(struct acpi_table_header *table)
{
	return ACPI_NAMECMP(ACPI_SIG_DSDT, table->signature) ||
	       ACPI_NAMECMP(ACPI_SIG_PSDT, table->signature) ||
	       ACPI_NAMECMP(ACPI_SIG_SSDT, table->signature);
}

uint32_t acpi_table_get_length(struct acpi_table_header *table)
{
	if (ACPI_RSDP_SIG_CMP(ACPI_CAST_RSDP(table)->signature))
		return ACPI_DECODE32(&(ACPI_CAST_RSDP(table)->length));
	else
		return ACPI_DECODE32(&(table->length));
}

/*=========================================================================
 * Non-RSDP table checksums
 *=======================================================================*/
static void __acpi_table_calc_checksum(struct acpi_table_header *table)
{
	ACPI_ENCODE8(&table->checksum, 0);
	ACPI_ENCODE8(&table->checksum,
		     acpi_checksum_calc(table, acpi_table_get_length(table)));
}

boolean acpi_table_has_header(acpi_name_t signature)
{
	return (!ACPI_NAMECMP(ACPI_SIG_S3PT, signature) &&
		!ACPI_NAMECMP(ACPI_SIG_FACS, signature));
}

boolean __acpi_table_checksum_valid(struct acpi_table_header *table)
{
	if (!acpi_table_has_header(table->signature))
		return true;

	return acpi_checksum_calc(table, acpi_table_get_length(table)) == 0;
}

boolean acpi_table_checksum_valid(struct acpi_table_header *table)
{
	if (ACPI_RSDP_SIG_CMP(table->signature))
		return acpi_rsdp_checksum_valid(ACPI_CAST_RSDP(table));
	else
		return __acpi_table_checksum_valid(table);
}

void acpi_table_calc_checksum(struct acpi_table_header *table)
{
	if (ACPI_RSDP_SIG_CMP(table->signature))
		acpi_rsdp_calc_checksum(ACPI_CAST_RSDP(table));
	else
		__acpi_table_calc_checksum(table);
}

static acpi_status_t acpi_table_list_resize(void)
{
	struct acpi_table_desc *tables;
	uint32_t table_count;

	if (!(acpi_gbl_table_list.flags & ACPI_ROOT_ALLOW_RESIZE))
		return AE_SUPPORT;

	if (acpi_gbl_table_list.flags & ACPI_ROOT_ORIGIN_ALLOCATED)
		table_count = acpi_gbl_table_list.max_table_count;
	else
		table_count = acpi_gbl_table_list.use_table_count;

	tables = heap_calloc((table_count + ACPI_TABLE_LIST_INCREMENT) *
			     sizeof (struct acpi_table_desc));
	if (!tables)
		return AE_NO_MEMORY;

	if (acpi_gbl_table_list.tables) {
		memcpy(tables, acpi_gbl_table_list.tables,
		       table_count * sizeof (struct acpi_table_desc));
		if (acpi_gbl_table_list.flags & ACPI_ROOT_ORIGIN_ALLOCATED)
			heap_free(acpi_gbl_table_list.tables);
	}

	acpi_gbl_table_list.tables = tables;
	acpi_gbl_table_list.max_table_count = table_count +
					      ACPI_TABLE_LIST_INCREMENT;
	acpi_gbl_table_list.flags |= ACPI_ROOT_ORIGIN_ALLOCATED;

	return AE_OK;
}

acpi_status_t acpi_reallocate_root_table(void)
{
	acpi_status_t status;

	if (acpi_gbl_table_list.flags & ACPI_ROOT_ORIGIN_ALLOCATED)
		return AE_SUPPORT;

	acpi_gbl_table_list.flags |= ACPI_ROOT_ALLOW_RESIZE;
	status = acpi_table_list_resize();

	return AE_OK;
}

acpi_status_t acpi_table_list_allocate(uint32_t initialial_table_count)
{
	acpi_status_t status;

	acpi_gbl_table_list.max_table_count = initialial_table_count;
	acpi_gbl_table_list.flags = ACPI_ROOT_ALLOW_RESIZE;
	status = acpi_table_list_resize();

	return status;
}

static acpi_status_t acpi_table_list_acquire(acpi_ddb_t *ddb_handle,
					     acpi_name_t name)
{
	acpi_status_t status;
	acpi_ddb_t ddb;

	if (ACPI_NAMECMP(ACPI_SIG_DSDT, name)) {
		ddb = ACPI_DDB_HANDLE_DSDT;
		goto out_succ;
	}
	if (ACPI_NAMECMP(ACPI_SIG_FACS, name)) {
		ddb = ACPI_DDB_HANDLE_FACS;
		goto out_succ;
	}
	foreach_table_handle_garbage(ddb)
		goto out_succ;

	if (acpi_gbl_table_list.use_table_count >= acpi_gbl_table_list.max_table_count) {
		status = acpi_table_list_resize();
		if (ACPI_FAILURE(status))
			return status;
	}
	ddb = acpi_gbl_table_list.use_table_count;
	acpi_gbl_table_list.use_table_count++;

out_succ:
	acpi_reference_inc(&acpi_gbl_table_list.all_table_count);
	*ddb_handle = ddb;
	acpi_gbl_table_list.tables[ddb].flags = 0;
	return AE_OK;
}

static acpi_status_t acpi_table_acquire(struct acpi_table_desc *table_desc,
					struct acpi_table_header **table_ptr,
					uint32_t *table_len,
					acpi_table_flags_t *table_flags)
{
	struct acpi_table_header *table = NULL;

	switch (table_desc->flags & ACPI_TABLE_ORIGIN_MASK) {
	case ACPI_TABLE_INTERNAL_PHYSICAL:
		table = acpi_os_map_memory(table_desc->address, table_desc->length);
		break;
	case ACPI_TABLE_EXTERNAL_VIRTUAL:
	case ACPI_TABLE_INTERNAL_VIRTUAL:
		table = ACPI_CAST_PTR(struct acpi_table_header,
				      ACPI_PHYSADDR_TO_PTR(table_desc->address));
		break;
	}
	if (!table)
		return AE_NO_MEMORY;

	*table_ptr = table;
	*table_len = table_desc->length;
	*table_flags = table_desc->flags;

	return AE_OK;
}

static void acpi_table_release(struct acpi_table_header *table,
			       uint32_t table_len,
			       acpi_table_flags_t table_flags)
{
	switch (table_flags & ACPI_TABLE_ORIGIN_MASK) {
	case ACPI_TABLE_INTERNAL_PHYSICAL:
		acpi_os_unmap_memory(table, table_len);
		break;
	case ACPI_TABLE_EXTERNAL_VIRTUAL:
	case ACPI_TABLE_INTERNAL_VIRTUAL:
	default:
		break;
	}
}

static acpi_status_t acpi_table_validate(struct acpi_table_desc *table_desc)
{
	acpi_status_t status = AE_OK;

	if (!table_desc->pointer)
		status = acpi_table_acquire(table_desc,
					    &table_desc->pointer,
					    &table_desc->length,
					    &table_desc->flags);

	return status;
}

static void acpi_table_invalidate(struct acpi_table_desc *table_desc)
{
	if (!table_desc->pointer)
		return;

	acpi_table_release(table_desc->pointer, table_desc->length,
			   table_desc->flags);
	table_desc->pointer = NULL;
}

static acpi_status_t acpi_table_verify(struct acpi_table_desc *table_desc,
				       acpi_tag_t signature)
{
	acpi_status_t status;

	status = acpi_table_validate(table_desc);
	if (ACPI_FAILURE(status))
		return status;
	if (!ACPI_NAMECMP(signature, ACPI_NULL_NAME) &&
	    !ACPI_NAMECMP(signature, table_desc->signature))
		return AE_BAD_SIGNATURE;
	if (!__acpi_table_checksum_valid(table_desc->pointer))
		return AE_BAD_CHECKSUM;

	return AE_OK;
}

static void __acpi_table_install(struct acpi_table_desc *table_desc,
				 acpi_addr_t address, acpi_table_flags_t flags,
				 struct acpi_table_header *table_header)
{
	memset(table_desc, 0, sizeof (struct acpi_table_desc));
	table_desc->address = address;
	table_desc->length = acpi_table_get_length(table_header);
	table_desc->flags = flags;
	ACPI_NAMECPY(ACPI_NAME2TAG(table_header->signature), table_desc->signature);
	if (acpi_table_has_header(table_header->signature)) {
		ACPI_OEMCPY(table_header->oem_id,
			    table_desc->oem_id, ACPI_OEM_ID_SIZE);
		ACPI_OEMCPY(table_header->oem_table_id,
			    table_desc->oem_table_id, ACPI_OEM_ID_SIZE);
		table_desc->revision = table_header->revision;
	}
}

static void acpi_table_uninstall_temporal(struct acpi_table_desc *table_desc)
{
	acpi_table_invalidate(table_desc);
}

static acpi_status_t acpi_table_install_temporal(struct acpi_table_desc *table_desc,
						 acpi_addr_t address, acpi_table_flags_t flags)
{
	struct acpi_table_header *table_header;

	switch (flags & ACPI_TABLE_ORIGIN_MASK) {
	case ACPI_TABLE_INTERNAL_PHYSICAL:
		table_header = acpi_os_map_memory(address, sizeof (struct acpi_table_header));
		if (!table_header)
			return AE_NO_MEMORY;
		__acpi_table_install(table_desc, address, flags, table_header);
		acpi_os_unmap_memory(table_header, sizeof (struct acpi_table_header));
		return AE_OK;
	case ACPI_TABLE_INTERNAL_VIRTUAL:
	case ACPI_TABLE_EXTERNAL_VIRTUAL:
		table_header = ACPI_CAST_PTR(struct acpi_table_header, address);
		__acpi_table_install(table_desc, address, flags, table_header);
		return AE_OK;
	}

	return AE_NO_MEMORY;
}

static void __acpi_table_uninstall(struct acpi_table_desc *table_desc)
{
	if (!table_desc->address)
		return;

	acpi_table_invalidate(table_desc);
	if ((table_desc->flags & ACPI_TABLE_ORIGIN_MASK) == ACPI_TABLE_INTERNAL_VIRTUAL)
		heap_free(ACPI_CAST_PTR(void, table_desc->address));
	table_desc->address = ACPI_PTR_TO_PHYSADDR(NULL);
}

static void acpi_table_override(struct acpi_table_desc *old_table_desc)
{
	acpi_status_t status;
	char *override_type;
	struct acpi_table_desc new_table_desc;
	acpi_addr_t address;
	acpi_table_flags_t flags;

	status = acpi_os_table_override(old_table_desc->pointer,
					&address, &flags);
	if (ACPI_SUCCESS(status) && address) {
		acpi_table_install_temporal(&new_table_desc, address, flags);
		override_type = "Logical";
		goto out_succ;
	}

	return;

out_succ:
	status = acpi_table_verify(&new_table_desc, ACPI_TAG_NULL);
	if (ACPI_FAILURE(status))
		return;

	__acpi_table_uninstall(old_table_desc);
	__acpi_table_install(old_table_desc, new_table_desc.address,
			     new_table_desc.flags, new_table_desc.pointer);
	acpi_table_validate(old_table_desc);
	acpi_table_uninstall_temporal(&new_table_desc);
}

static void acpi_determine_integer_width(uint8_t revision)
{
	if (revision < 2)
		acpi_gbl_integer_bit_width = 32;
	else
		acpi_gbl_integer_bit_width = 64;
}

static void acpi_table_install_and_override(struct acpi_table_desc *new_table_desc,
					    acpi_ddb_t ddb, boolean override)
{
	if (acpi_gbl_table_finalizing)
		return;

	if (ddb >= acpi_gbl_table_list.use_table_count)
		return;

	if (override)
		acpi_table_override(new_table_desc);

	__acpi_table_install(&acpi_gbl_table_list.tables[ddb],
			     new_table_desc->address, new_table_desc->flags,
			     new_table_desc->pointer);
	/* Acquire the MANAGED reference */
	acpi_table_increment(ddb);
	acpi_table_notify(&acpi_gbl_table_list.tables[ddb], ddb,
			  ACPI_EVENT_TABLE_INSTALL);

	if (ddb == ACPI_DDB_HANDLE_DSDT)
		acpi_determine_integer_width(ACPI_DECODE8(&new_table_desc->pointer->revision));
}

acpi_status_t acpi_table_install_fixed(acpi_addr_t address,
				       acpi_tag_t signature,
				       acpi_ddb_t ddb)
{
	struct acpi_table_desc new_table_desc;
	acpi_status_t status;

	if (!address) {
		acpi_err("[0x%X]: Null physical address for ACPI table", signature);
		return AE_NO_MEMORY;
	}

	status = acpi_table_install_temporal(&new_table_desc, address, ACPI_TABLE_INTERNAL_PHYSICAL);
	if (ACPI_FAILURE(status)) {
		acpi_err("[0x%X]: Could not acquire table length at %p", signature,
			 ACPI_CAST_PTR(void, address));
		return status;
	}

	status = acpi_table_verify(&new_table_desc, signature);
	if (ACPI_FAILURE(status))
		goto err_inst;

	acpi_table_lock();
	acpi_table_install_and_override(&new_table_desc, ddb, true);
	acpi_table_unlock();

err_inst:
	acpi_table_uninstall_temporal(&new_table_desc);
	return status;
}

boolean acpi_table_is_same(struct acpi_table_desc *table_desc, acpi_tag_t sig,
			   char *oem_id, char *oem_table_id)
{
	if (ACPI_NAMECMP(ACPI_SIG_DSDT, table_desc->signature) ||
	    ACPI_NAMECMP(ACPI_SIG_FADT, table_desc->signature) ||
	    !acpi_table_has_header(table_desc->signature))
		return ACPI_NAMECMP(sig, table_desc->signature) ? true: false;

	return ((ACPI_NAMECMP(sig, table_desc->signature) &&
		 ACPI_OEMCMP(oem_id, table_desc->oem_id, ACPI_OEM_ID_SIZE)&&
		 ACPI_OEMCMP(oem_table_id, table_desc->oem_table_id, ACPI_OEM_TABLE_ID_SIZE)) ?
		true : false);
}

acpi_status_t acpi_table_install_non_fixed(acpi_addr_t address,
					   acpi_table_flags_t flags,
					   boolean reload,
					   acpi_ddb_t *ddb_handle)
{
	acpi_ddb_t ddb;
	acpi_status_t status;
	struct acpi_table_desc new_table_desc;
	struct acpi_table_desc *table_desc;

	status = acpi_table_install_temporal(&new_table_desc, address, flags);
	if (ACPI_FAILURE(status))
		return status;

	status = acpi_table_verify(&new_table_desc, ACPI_TAG_NULL);
	if (ACPI_FAILURE(status))
		return status;

	acpi_table_lock();
	if (reload) {
		foreach_table_handle(ddb, 0) {
			table_desc = &acpi_gbl_table_list.tables[ddb];
			if (!acpi_table_is_same(table_desc,
						ACPI_NAME2TAG(new_table_desc.signature),
						new_table_desc.oem_id,
						new_table_desc.oem_table_id))
				continue;
			if (new_table_desc.revision <= table_desc->revision) {
				status = AE_ALREADY_EXISTS;
				goto err_lock;
			}

			acpi_uninstall_table(ddb);
		}
	}

	status = acpi_table_list_acquire(&ddb, new_table_desc.signature);
	if (ACPI_FAILURE(status))
		goto err_lock;

	/* Wait until uninstall completes, for FACS and DSDT reloading */
	if (reload &&
	    (ACPI_NAMECMP(ACPI_SIG_DSDT, new_table_desc.signature) ||
	     ACPI_NAMECMP(ACPI_SIG_FACS, new_table_desc.signature))) {
		while (acpi_reference_get(&acpi_gbl_table_list.tables[ddb].reference_count))
			acpi_os_sleep(10);
	}

	*ddb_handle = ddb;
	acpi_table_install_and_override(&new_table_desc, ddb, (boolean)!reload);

err_lock:
	acpi_table_unlock();
	acpi_table_uninstall_temporal(&new_table_desc);
	return status;
}

boolean acpi_table_is_loaded(acpi_ddb_t ddb)
{
	if (ddb < acpi_gbl_table_list.use_table_count &&
	    acpi_gbl_table_list.tables[ddb].flags & ACPI_TABLE_IS_LOADED)
		return true;
	return false;
}

void acpi_table_set_loaded(acpi_ddb_t ddb, boolean is_loaded)
{
	if (ddb < acpi_gbl_table_list.use_table_count) {
		if (is_loaded) {
			acpi_table_increment(ddb);
			acpi_gbl_table_list.tables[ddb].flags |= ACPI_TABLE_IS_LOADED;
		} else {
			acpi_table_decrement(ddb);
			acpi_gbl_table_list.tables[ddb].flags &= ~ACPI_TABLE_IS_LOADED;
		}
	}
}

acpi_status_t acpi_table_parse(acpi_ddb_t ddb,
			       struct acpi_namespace_node *start_node)
{
	acpi_status_t status;

	status = acpi_parse_once(ACPI_IMODE_LOAD_PASS1, ddb, start_node);
	if (ACPI_FAILURE(status))
		return status;

	return acpi_parse_once(ACPI_IMODE_LOAD_PASS2, ddb, start_node);
}

acpi_status_t __acpi_load_table(acpi_ddb_t ddb,
				struct acpi_namespace_node *node)
{
	acpi_status_t status = AE_OK;

	if (acpi_table_is_loaded(ddb))
		return AE_ALREADY_EXISTS;

	status = acpi_table_parse(ddb, node);
	if (ACPI_SUCCESS(status)) {
		acpi_table_set_loaded(ddb, true);
		acpi_table_notify(&acpi_gbl_table_list.tables[ddb], ddb,
				  ACPI_EVENT_TABLE_LOAD);
	}

	return status;
	/* return AcpiDsInitializeObjects(ddb, node); */
}

void acpi_table_increment(acpi_ddb_t ddb)
{
	struct acpi_table_desc *table_desc;

	BUG_ON(ddb >= acpi_gbl_table_list.use_table_count);
	table_desc = &acpi_gbl_table_list.tables[ddb];
	acpi_reference_inc(&table_desc->reference_count);
}

void acpi_table_decrement(acpi_ddb_t ddb)
{
	struct acpi_table_desc *table_desc;

	table_desc = &acpi_gbl_table_list.tables[ddb];
	if (table_desc &&
	    acpi_reference_dec_and_test(&table_desc->reference_count) == 0) {
		table_desc->flags |= ACPI_TABLE_IS_GARBAGE;
		__acpi_table_uninstall(table_desc);
		acpi_reference_dec(&acpi_gbl_table_list.all_table_count);
	}
}

static acpi_status_t __acpi_get_table(acpi_ddb_t ddb,
				      struct acpi_table_header **out_table)
{
	acpi_status_t status;
	struct acpi_table_desc *table_desc;
	struct acpi_table_header *table;
	uint32_t table_len;
	acpi_table_flags_t table_flags;

	acpi_table_increment(ddb);
	table_desc = &acpi_gbl_table_list.tables[ddb];
	acpi_table_unlock();

	if (table_desc->flags & ACPI_TABLE_IS_GARBAGE)
		status = AE_NOT_FOUND;
	else {
		if (table_desc->pointer) {
			*out_table = table_desc->pointer;
			status = AE_OK;
		} else {
			status = acpi_table_acquire(table_desc,
						    &table,
						    &table_len,
						    &table_flags);
			if (ACPI_SUCCESS(status))
				*out_table = table;
		}
	}

	if (ACPI_FAILURE(status))
		acpi_table_decrement(ddb);
	acpi_table_lock();

	return status;
}

acpi_status_t acpi_get_table(acpi_ddb_t ddb, struct acpi_table_header **out_table)
{
	acpi_status_t status;

	if (!out_table)
		return AE_BAD_PARAMETER;
	*out_table = NULL;

	acpi_table_lock();
	if (ddb >= acpi_gbl_table_list.use_table_count) {
		status = AE_NOT_FOUND;
		goto err_lock;
	}

	status = __acpi_get_table(ddb, out_table);

err_lock:
	acpi_table_unlock();
	return status;
}

void acpi_put_table(acpi_ddb_t ddb, struct acpi_table_header *table)
{
	struct acpi_table_desc *table_desc;

	table_desc = &acpi_gbl_table_list.tables[ddb];

	if (!table_desc->pointer)
		acpi_table_release(table, table_desc->length, table_desc->flags);
	acpi_table_decrement(ddb);
}

acpi_status_t acpi_get_table_by_inst(acpi_tag_t sig, uint32_t instance,
				     acpi_ddb_t *ddb_handle,
				     struct acpi_table_header **out_table)
{
	acpi_status_t status = AE_NOT_FOUND;
	acpi_ddb_t ddb;
	uint32_t j = 0;
	struct acpi_table_desc *table_desc;

	if (ACPI_NAMECMP(sig,  ACPI_NULL_NAME) ||
	    !ddb_handle || !out_table)
		return AE_BAD_PARAMETER;
	*out_table = NULL;

	acpi_table_lock();
	foreach_table_handle(ddb, 0) {
		table_desc = &acpi_gbl_table_list.tables[ddb];

		if (!ACPI_NAMECMP(sig, table_desc->signature))
			continue;
		if (++j < instance)
			continue;

		*ddb_handle = ddb;
		status = __acpi_get_table(ddb, out_table);
		break;
	}
	acpi_table_unlock();

	return status;
}

acpi_status_t acpi_get_table_by_name(acpi_tag_t sig, char *oem_id, char *oem_table_id,
				     acpi_ddb_t *ddb_handle,
				     struct acpi_table_header **out_table)
{
	acpi_status_t status = AE_NOT_FOUND;
	acpi_ddb_t ddb;
	struct acpi_table_desc *table_desc;

	if (ACPI_NAMECMP(sig,  ACPI_NULL_NAME) ||
	    !oem_id || !oem_table_id || !ddb_handle || !out_table)
		return AE_BAD_PARAMETER;
	*out_table = NULL;

	acpi_table_lock();
	foreach_table_handle(ddb, 0) {
		table_desc = &acpi_gbl_table_list.tables[ddb];

		if (!acpi_table_is_same(table_desc, sig, oem_id, oem_table_id))
			continue;

		*ddb_handle = ddb;
		status = __acpi_get_table(ddb, out_table);
		break;
	}
	acpi_table_unlock();

	return status;
}

acpi_status_t acpi_install_table(struct acpi_table_header *table,
				 acpi_table_flags_t flags,
				 acpi_ddb_t *ddb_handle)
{
	acpi_status_t status;
	
	if (!table || !ddb_handle)
		return AE_BAD_PARAMETER;
	
	status = acpi_table_install_non_fixed(ACPI_PTR_TO_PHYSADDR(table),
					      flags, true, ddb_handle);
	return status;
}

void acpi_load_tables(void)
{
	acpi_ddb_t ddb;

	acpi_table_lock();

	foreach_table_handle(ddb, 0) {
		if ((!ACPI_NAMECMP(ACPI_SIG_DSDT,
				   acpi_gbl_table_list.tables[ddb].signature) &&
		     !ACPI_NAMECMP(ACPI_SIG_SSDT,
				   acpi_gbl_table_list.tables[ddb].signature) &&
		     !ACPI_NAMECMP(ACPI_SIG_PSDT,
				   acpi_gbl_table_list.tables[ddb].signature)) ||
		    ACPI_FAILURE(acpi_table_validate(&acpi_gbl_table_list.tables[ddb])))
			continue;

		acpi_table_increment(ddb);
		acpi_table_unlock();
		(void)__acpi_load_table(ddb, acpi_gbl_root_node);
		acpi_table_decrement(ddb);
		acpi_table_lock();
	}

	acpi_table_unlock();
}

void acpi_uninstall_table(acpi_ddb_t ddb)
{
	struct acpi_table_desc *table_desc = &acpi_gbl_table_list.tables[ddb];

	if (acpi_table_is_loaded(ddb)) {
		acpi_table_notify(table_desc, ddb,
				  ACPI_EVENT_TABLE_UNLOAD);
		/* acpi_table_unparse(ddb); */
		acpi_table_set_loaded(ddb, false);
	}

	/* Release the MANAGED reference */
	acpi_table_notify(&acpi_gbl_table_list.tables[ddb], ddb,
			  ACPI_EVENT_TABLE_UNINSTALL);
	acpi_table_decrement(ddb);
}

acpi_status_t acpi_initialize_tables(struct acpi_table_desc *initial_table_array,
				     uint32_t initial_table_count,
				     boolean allow_resize)
{
	acpi_addr_t rsdp_address;
	acpi_status_t status;

	status = acpi_os_create_mutex(&acpi_gbl_table_mutex);
	if (ACPI_FAILURE(status))
		return status;

	rsdp_address = acpi_os_get_root_pointer();
	if (!rsdp_address)
		return AE_NOT_FOUND;

	acpi_table_lock();
	if (!initial_table_array) {
		status = acpi_table_list_allocate(initial_table_count);
		if (status)
			return status;
	} else {
		memset(initial_table_array, 0,
		       initial_table_count * sizeof (struct acpi_table_desc));

		acpi_gbl_table_list.tables = initial_table_array;
		acpi_gbl_table_list.max_table_count = initial_table_count;
		acpi_gbl_table_list.flags = ACPI_ROOT_ORIGIN_UNKNOWN;
		if (allow_resize)
			acpi_gbl_table_list.flags |= ACPI_ROOT_ALLOW_RESIZE;
	}
	acpi_reference_set(&acpi_gbl_table_list.all_table_count,
			   ACPI_TABLE_LIST_FIXED);
	acpi_gbl_table_list.use_table_count = ACPI_TABLE_LIST_FIXED;
	acpi_table_unlock();

	return acpi_rsdp_parse(rsdp_address);
}

void acpi_finalize_tables(void)
{
	acpi_ddb_t ddb;

	acpi_table_lock();
	acpi_gbl_table_finalizing = true;
	foreach_table_handle(ddb, 0) {
		acpi_table_increment(ddb);
		acpi_table_unlock();
		acpi_uninstall_table(ddb);
		acpi_table_decrement(ddb);
		acpi_table_lock();
	}

	while (acpi_reference_get(&acpi_gbl_table_list.all_table_count) != 0) {
		acpi_table_unlock();
		acpi_os_sleep(10);
		acpi_table_lock();
	}

	if (acpi_gbl_table_list.flags & ACPI_ROOT_ORIGIN_ALLOCATED)
		heap_free(acpi_gbl_table_list.tables);
	acpi_gbl_table_list.tables = NULL;
	acpi_gbl_table_list.flags = 0;
	acpi_gbl_table_list.use_table_count = 0;
	acpi_gbl_table_list.max_table_count = 0;
	acpi_table_unlock();
}
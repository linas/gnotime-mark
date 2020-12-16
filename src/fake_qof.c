#include "fake_qof.h"

QofDateFormat
qof_date_format_get_current(void)
{
	return QOF_DATE_FORMAT_CUSTOM;
}

const GUID *
qof_entity_get_guid(G_GNUC_UNUSED void *a)
{
	return NULL;
}

void
qof_entity_set_guid(G_GNUC_UNUSED void *a, G_GNUC_UNUSED const GUID *b)
{
}

void
qof_instance_init(G_GNUC_UNUSED void *a, G_GNUC_UNUSED const char *b,
                  G_GNUC_UNUSED QofBook *c)
{
}

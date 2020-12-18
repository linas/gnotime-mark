#include "fake_qof.h"

QofBook *
qof_book_new()
{
	return NULL;
}

void
qof_class_register(G_GNUC_UNUSED const char *a, G_GNUC_UNUSED QofSortFunc b,
                   G_GNUC_UNUSED QofParam *c)
{
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
qof_instance_init(G_GNUC_UNUSED QofInstance *a, G_GNUC_UNUSED const char *b,
                  G_GNUC_UNUSED const QofBook *c)
{
}

gboolean
qof_object_register(G_GNUC_UNUSED QofObject *a)
{
	return FALSE;
}

QofTime *
qof_time_new()
{
	return NULL;
}

void
qof_time_set_secs(G_GNUC_UNUSED QofTime *a, G_GNUC_UNUSED time_t b)
{
}

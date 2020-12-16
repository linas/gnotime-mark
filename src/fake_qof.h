#ifndef FAKE_QOF_H_
#define FAKE_QOF_H_

#include <glib-object.h>
#include <glib.h>

#define QOF_UTC_DATE_FORMAT "%Y-%m-%dT%H:%M:%SZ"

#define QOF_DATE_FORMAT_US 1
#define QOF_DATE_FORMAT_UK 2
#define QOF_DATE_FORMAT_CE 3
#define QOF_DATE_FORMAT_ISO 4
#define QOF_DATE_FORMAT_UTC 5
#define QOF_DATE_FORMAT_ISO8601 6
#define QOF_DATE_FORMAT_LOCALE 7
#define QOF_DATE_FORMAT_CUSTOM 8

typedef gint QofDateFormat;

#define GUID_DATA_SIZE 16
typedef union _GUID {
	guchar data[GUID_DATA_SIZE];

	gint __align_me; /* this just ensures that GUIDs are 32-bit
	                  * aligned on systems that need them to be. */
} GUID;

struct QofInstance_s
{
	GObject object;

	/* QofIdType e_type; */ /**< Entity type */

	/* kvp_data is a key-value pair database for storing arbirtary
	 * information associated with this instance.
	 * See src/engine/kvp_doc.txt for a list and description of the
	 * important keys. */
	/* KvpFrame *kvp_data; */

	void *entity;
};

typedef struct QofInstance_s QofInstance;

struct _QofBook
{
};

typedef struct _QofBook QofBook;

QofDateFormat qof_date_format_get_current(void);
const GUID *qof_entity_get_guid(void *);
void qof_entity_set_guid(void *, const GUID *);
void qof_instance_init(void *, const char *, QofBook *);

#endif /* FAKE_QOF_H_ */

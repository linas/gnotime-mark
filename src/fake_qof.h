#ifndef FAKE_QOF_H_
#define FAKE_QOF_H_

#include <glib-object.h>
#include <glib.h>

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
};

typedef struct QofInstance_s QofInstance;

#endif /* FAKE_QOF_H_ */

#ifndef QTT_FAKE_QOF_H_
#define QTT_FAKE_QOF_H_

#include <glib.h>

#define QOF_OBJECT_VERSION 3
#define QOF_TYPE_TIME "time"

typedef const gchar *QofIdType;
typedef const char *QofType;
typedef struct _QofParam QofParam;
typedef struct _QofTime QofTime;

#define GUID_DATA_SIZE 16
typedef union _GUID {
	guchar data[GUID_DATA_SIZE];

	gint __align_me;
} GUID;

struct _QofBook
{
};
typedef struct _QofBook QofBook;

struct QofCollection_s
{
	QofIdType e_type;
	gboolean is_dirty;

	GHashTable *hash_of_entries;
	gpointer data;
};
typedef struct QofCollection_s QofCollection;

struct _QofEntityTable
{
	GHashTable *hash;
};
typedef struct _QofEntityTable QofEntityTable;

struct QofEntity_s
{
	QofIdType e_type;
	GUID guid;
	QofEntityTable *e_table;
};
typedef struct QofEntity_s QofEntity;

struct QofInstance_s
{
	void *entity;
};
typedef struct QofInstance_s QofInstance;

typedef void (*QofInstanceForeachCB)(QofInstance *, gpointer user_data);

struct _QofObject
{
	gint interface_version;
	QofIdType e_type;
	const char *type_label;
	gpointer (*create)(QofBook *);
	void (*book_begin)(QofBook *);
	void (*book_end)(QofBook *);
	gboolean (*is_dirty)(const QofCollection *);
	void (*mark_clean)(QofCollection *);
	void (*foreach)(const QofCollection *, QofInstanceForeachCB, gpointer);
	const char *(*printable)(gpointer instance);
	int (*version_cmp)(gpointer instance_left, gpointer instance_right);
};
typedef struct _QofObject QofObject;

struct _QofTime
{
};

typedef gpointer (*QofAccessFunc)(gpointer object,
                                  /*@ null @*/ const QofParam *param);
typedef gint (*QofCompareFunc)(gpointer a, gpointer b, gint compare_options,
                               QofParam *getter);
typedef void (*QofSetterFunc)(gpointer, /*@ null @*/ gpointer);

struct _QofParam
{
	const char *param_name;
	QofTime param_type;
	QofAccessFunc param_getfcn;
	QofSetterFunc param_setfcn;
	QofCompareFunc param_compfcn;
	gpointer param_userdata;
};

typedef void (*QofEntityForeachCB)(gpointer object, gpointer user_data);
typedef int (*QofSortFunc)(gconstpointer, gconstpointer);

QofBook *qof_book_new();
void qof_class_register(const char *, QofSortFunc, QofParam *);
const GUID *qof_entity_get_guid(void *);
void qof_entity_set_guid(void *, const GUID *);
void qof_instance_init(QofInstance *, const char *, const QofBook *);
gboolean qof_object_register(QofObject *);
QofTime *qof_time_new();
void qof_time_set_secs(QofTime *, time_t);

#endif /* QTT_FAKE_QOF_H_ */

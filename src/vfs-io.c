
/* This file contains sample/experimental hacking code for a gnomevfs client.
 * it allows playing with the API.  It shows how to use the GnomeVFS API
 * in a client such as GnoTime.
 *
 * This is not a part of the standard GnoTime distribution, its just for hacking around.
 */
/* ------------------------------------------------------------------ */
#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <sys/types.h>

/* See http://developer.gnome.org/doc/API/2.0/gnome-vfs-2.0/
 * for a good set of API documentation for GnomeVFS clients.
 *
 * See http://www-106.ibm.com/developerworks/linux/library/l-gnvfs/?ca=dnt-435
 * for an example on how to write a gnomevfs server aka 'module'
 *
 * http://www.stafford.uklinux.net/libesmtp/
 */

void
gtt_save_buffer (const gchar *uri, const char * buf)
{
	GFile * const file_uri = g_file_new_for_uri(uri);

	const gboolean exists = g_file_query_exists (file_uri);
	if (exists)
	{
		// file exists are you sure you want to over-write?
	}

	GError * err = NULL;
	file_ostream = g_file_append_to(file_uri, G_FILE_CREATE_NONE, NULL, &err);

	if (!file_ostream || err)
	{
		printf ("duuude pen error=%d\n", err->code);
		return;
	}

	gsize buflen = strlen(buf);
	gssize bytes_written = 0;
	gsize off = 0;
	while ((0 <= bytes_written) && !err)
	{
		bytes_written = g_output_stream_write(file_ostream, &buf[off], buflen, NULL, &err);
		off += bytes_written;
		buflen -= bytes_written;

		printf ("duude wrote %lld bytes left=%d\n", bytes_written, buflen);
		if (0>= buflen) break;
	}
	if (err)
	{
		printf ("duuude write error=%d\n", err->code);
		return;
	}
	if (!g_output_stream_close(G_OUTPUT_STREAM(file_ostream), NULL, &err) || err) {
		/* TODO: Report error */
	}
	g_object_unref (file_uri);
}

#define BUF_SIZE 8192

void
load_gorp (const gchar *uri)
{
	gchar             buffer[BUF_SIZE];

	GError * err = NULL;
	GFile * const file_obj = g_file_new_for_uri(uri);
	GFileInputStream * const file_istream = g_file_read(file_obj, NULL, &err);
	if (!file_istream || err) {
		/* TODO: Handle or report error */
	}

	printf ("duude attempt to read %s\n", uri);
	while (!err)
	{
		g_input_stream_read(G_INPUT_STREAM(file-istream), buffer, BUF_SIZE, NULL, &err);

		printf ("duude got %lld %s\n", bytes_read, buffer);
	}
	if (!g_input_stream_close(G_INPUT_STREAM(file_istream), NULL, &err) || err) {
		/* TODO: Handle or report error */
	}
	g_object_unref(file_obj);
}

int
main (int argc, char **argv)
{
	if (argc < 2) {
	         g_print ("Run with %s <uri>\n", argv[0]);
	          exit (1);
	}

	 // load_gorp (argv[1]);
	 char *msg="qewrtyuiop duuuude";
	 gtt_save_buffer (argv[1], msg);

	  return 0;
}
/* ------------------------------------------------------------------ */


/*
 To compile the example:

# gcc `pkg-config --libs --cflags gtk+-2.0 gnome-vfs-2.0` j.c

*/

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnomevfs-monitor.c - Test for file/directory monitoring in gnome-vfs

   Copyright (C) 2005 Christian Kellner

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Chrsitian Kellner <gicmo@gnome.org>
*/

#include <libgnomevfs/gnome-vfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "authentication.c"

static void  
monitor_event (GnomeVFSMonitorHandle *handle,
               const gchar *monitor_uri,
	       const gchar *info_uri,
	       GnomeVFSMonitorEventType event_type,
	       gpointer user_data)
{
	if (! g_str_equal ((char *) user_data, "user_data")) {
		g_critical ("user_data corruption\n");
	}
	
	switch (event_type) {

		case GNOME_VFS_MONITOR_EVENT_CHANGED:
			g_print ("Changed event:\n");
			break;

		case GNOME_VFS_MONITOR_EVENT_DELETED:
			g_print ("Deleted event:\n");
			break;

		case GNOME_VFS_MONITOR_EVENT_STARTEXECUTING:
			g_print ("Start executing event:\n");
			break;

		case GNOME_VFS_MONITOR_EVENT_STOPEXECUTING:
			g_print ("Stop executing event:\n");
			break;

		case GNOME_VFS_MONITOR_EVENT_CREATED:
			g_print ("Created event:\n");
			break;

		case GNOME_VFS_MONITOR_EVENT_METADATA_CHANGED:
			g_print ("Metadata changed event:\n");
			break;
		
		default:
			g_assert_not_reached ();
	}

	g_print ("  -> monitor_uri: %s\n", monitor_uri);
	g_print ("  -> info_uri: %s\n", info_uri);
}

static GMainLoop *loop = NULL;

static void stop_mainloop (int s)
{
	g_print ("Stopping monitor\n");
	g_main_loop_quit (loop);
}


int
main (int argc, char **argv)
{
	GnomeVFSResult         result;
	GnomeVFSMonitorType    mtype;
	GnomeVFSMonitorHandle *handle;
	GnomeVFSFileInfo      *info;
	char                  *text_uri;

	if (argc != 2) {
		printf ("Usage: %s <location>\n", argv[0]);
		return 1;
	}

	if (!gnome_vfs_init ()) {
		fprintf (stderr, "Cannot initialize gnome-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();

	text_uri = gnome_vfs_make_uri_from_shell_arg (argv[1]);

	if (text_uri == NULL) {
		fprintf (stderr, "Could not create uri from location\n");
		return -1;
	}
	
	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (text_uri, info, 0);
	
	if (result != GNOME_VFS_OK) {
		fprintf (stderr, "Could not get file info for %s\n", text_uri);
		g_free (text_uri);
		return -1;
	}
	
	g_print ("Starting monitor for %s\n", text_uri);
	
	if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		mtype = GNOME_VFS_MONITOR_DIRECTORY;
	} else {
		mtype = GNOME_VFS_MONITOR_FILE;
	}
	
	gnome_vfs_file_info_unref (info);	
	result = gnome_vfs_monitor_add (&handle, 
					text_uri, 
					mtype, 
					monitor_event, 
					"user_data");
	
	if (result != GNOME_VFS_OK) {
		fprintf (stderr, "Could not start monitor for %s\n", text_uri);
		g_free (text_uri);
		return -1;
	}
	
	loop = g_main_loop_new (NULL, FALSE);
	signal (SIGINT, stop_mainloop);
	
	g_main_loop_run (loop);
	
	gnome_vfs_monitor_cancel (handle);	
	g_free (text_uri);
	gnome_vfs_shutdown ();
	
	return 0;
}
	

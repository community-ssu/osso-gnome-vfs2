/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-directory.c - Directory handling for the GNOME Virtual
   File System.

   Copyright (C) 1999 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@gnu.org> */

#include <config.h>
#include "gnome-vfs-directory.h"

#include "gnome-vfs-cancellable-ops.h"
#include "gnome-vfs-method.h"
#include "gnome-vfs-ops.h"
#include <glib/gmessages.h>
#include <glib/gstrfuncs.h>

#define VFS_MAXIMUM_SYMBOLIC_LINK_DEPTH 256

struct GnomeVFSDirectoryHandle {
	/* URI of the directory being accessed through the handle.  */
	GnomeVFSURI *uri;

	/* Options.  */
	GnomeVFSFileInfoOptions options;

	/* Method-specific handle.  */
	GnomeVFSMethodHandle *method_handle;
};

#define CHECK_IF_SUPPORTED(vfs_method, what)		\
G_STMT_START{						\
	if (!VFS_METHOD_HAS_FUNC(vfs_method, what))			\
		return GNOME_VFS_ERROR_NOT_SUPPORTED;	\
}G_STMT_END

static GnomeVFSDirectoryHandle *
gnome_vfs_directory_handle_new (GnomeVFSURI *uri,
				GnomeVFSMethodHandle *method_handle,
				GnomeVFSFileInfoOptions options)
{
	GnomeVFSDirectoryHandle *new;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (method_handle != NULL, NULL);

	new = g_new (GnomeVFSDirectoryHandle, 1);

	gnome_vfs_uri_ref (uri);

	new->uri = uri;
	new->method_handle = method_handle;
	new->options = options;

	return new;
}

static void
gnome_vfs_directory_handle_destroy (GnomeVFSDirectoryHandle *handle)
{
	g_return_if_fail (handle != NULL);

	gnome_vfs_uri_unref (handle->uri);

	g_free (handle);
}

static GnomeVFSResult
open_from_uri (GnomeVFSDirectoryHandle **handle,
	       GnomeVFSURI *uri,
	       GnomeVFSFileInfoOptions options,
	       GnomeVFSContext *context)
{
	GnomeVFSMethodHandle *method_handle;
	GnomeVFSResult result;

	CHECK_IF_SUPPORTED (uri->method, open_directory);


	result = uri->method->open_directory (uri->method, 
					      &method_handle, 
					      uri,
					      options, 
					      context);
	if (result != GNOME_VFS_OK) {
		return result;
	}

	*handle = gnome_vfs_directory_handle_new (uri,
						  method_handle,
						  options);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_open (GnomeVFSDirectoryHandle **handle,
	 const gchar *text_uri,
	 GnomeVFSFileInfoOptions options,
	 GnomeVFSContext *context)
{
	GnomeVFSURI *uri;
	GnomeVFSResult result;

	uri = gnome_vfs_uri_new (text_uri);
	if (uri == NULL)
		return GNOME_VFS_ERROR_INVALID_URI;

	result = open_from_uri (handle, uri, options,
				context);

	gnome_vfs_uri_unref (uri);

	return result;
}

/**
 * gnome_vfs_directory_open:
 * @handle: pointer to a pointer to a #GnomeVFSDirectoryHandle object.
 * @text_uri: string representing the uri to open.
 * @options: options for reading file information.
 * 
 * Open directory @text_uri for reading.  On return, @*handle will point to
 * a #GnomeVFSDirectoryHandle object which can be used to read the directory
 * entries one by one.
 * 
 * Returns: an integer representing the result of the operation.
 */
GnomeVFSResult
gnome_vfs_directory_open (GnomeVFSDirectoryHandle **handle,
			  const gchar *text_uri,
			  GnomeVFSFileInfoOptions options)
{
	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	*handle = NULL;
	g_return_val_if_fail (text_uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	return do_open (handle, text_uri, options, NULL);
}

/**
 * gnome_vfs_directory_open_from_uri:
 * @handle: pointer to a pointer to a #GnomeVFSDirectoryHandle object.
 * @uri: uri to open.
 * @options: options for reading file information.
 * 
 * Open directory @text_uri for reading.  On return, @*handle will point to
 * a #GnomeVFSDirectoryHandle object which can be used to read the directory
 * entries one by one.
 * 
 * Returns: an integer representing the result of the operation.
 */
GnomeVFSResult
gnome_vfs_directory_open_from_uri (GnomeVFSDirectoryHandle **handle,
				   GnomeVFSURI *uri,
				   GnomeVFSFileInfoOptions options)
{
	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	*handle = NULL;
	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	return open_from_uri (handle, uri, options, NULL);
}

GnomeVFSResult
gnome_vfs_directory_open_from_uri_cancellable (GnomeVFSDirectoryHandle **handle,
				   GnomeVFSURI *uri,
				   GnomeVFSFileInfoOptions options,
				   GnomeVFSContext *context)
{
	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	*handle = NULL;
	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	return open_from_uri (handle, uri, options, context);
}

/**
 * gnome_vfs_directory_read_next:
 * @handle: a directory handle.
 * @file_info: pointer to a #GnomeVFSFileInfo struct where the data about
 * the directory at @handle will be stored.
 * 
 * Read the next directory entry from @handle.
 * 
 * Returns: an integer value representing the result of the operation.
 */
GnomeVFSResult
gnome_vfs_directory_read_next (GnomeVFSDirectoryHandle *handle,
			       GnomeVFSFileInfo *file_info)
{
	CHECK_IF_SUPPORTED (handle->uri->method, read_directory);

	gnome_vfs_file_info_clear (file_info);
	return handle->uri->method->read_directory (handle->uri->method,
						    handle->method_handle,
						    file_info, NULL);
}

/**
 * gnome_vfs_directory_read_next_cancellable:
 * @handle: a directory handle.
 * @file_info: pointer to a #GnomeVFSFileInfo struct where the data about 
 * the directory at @handle will be stored.
 * @context: a #GnomeVFSContext.
 *
 * Read the next directory entry from @handle.
 *
 * Returns: an integer value representing the result of the operation.
 */
GnomeVFSResult 
gnome_vfs_directory_read_next_cancellable (GnomeVFSDirectoryHandle *handle,
					   GnomeVFSFileInfo *file_info,
					   GnomeVFSContext *context)
{
	CHECK_IF_SUPPORTED (handle->uri->method, read_directory);

	gnome_vfs_file_info_clear (file_info);
	return handle->uri->method->read_directory (handle->uri->method,
						    handle->method_handle,
						    file_info, context);
}


/**
 * gnome_vfs_directory_close:
 * @handle: a directory handle.
 * 
 * Close @handle.
 * 
 * Returns: an integer representing the result of the operation.
 */
GnomeVFSResult
gnome_vfs_directory_close (GnomeVFSDirectoryHandle *handle)
{
	GnomeVFSResult result;

	CHECK_IF_SUPPORTED (handle->uri->method, close_directory);

	result = handle->uri->method->close_directory (handle->uri->method,
						       handle->method_handle,
						       NULL);

	gnome_vfs_directory_handle_destroy (handle);

	return result;
}


struct _DirectoryReference {
	ino_t inode;
	dev_t device;
};
typedef struct _DirectoryReference DirectoryReference;

static GList *
prepend_reference (GList *reference_list,
		   GnomeVFSFileInfo *info)
{
	DirectoryReference *reference;

	reference = g_new (DirectoryReference, 1);
	reference->device = info->device;
	reference->inode = info->inode;

	return g_list_prepend (reference_list, reference);
}

static GList *
remove_first_reference (GList *reference_list)
{
	GList *first;

	if (reference_list == NULL)
		return NULL;

	first = reference_list;
	g_free (first->data);

	reference_list = g_list_remove_link (reference_list, first);
	g_list_free (first);

	return reference_list;
}

static gboolean
lookup_ancestor (GList *ancestors,
		 gboolean inode_and_device_are_valid,
		 ino_t inode,
		 dev_t device)
{
	GList *p;

	if (!inode_and_device_are_valid) {
		return g_list_length (ancestors) >= VFS_MAXIMUM_SYMBOLIC_LINK_DEPTH;
	}

	for (p = ancestors; p != NULL; p = p->next) {
		DirectoryReference *reference;

		reference = p->data;
		if (reference->inode == inode && reference->device == device)
			return TRUE;
	}

	return FALSE;
}

static GnomeVFSResult
directory_visit_internal (GnomeVFSURI *uri,
			  const gchar *prefix,
			  GList *ancestor_references, /* DirectoryReference */
			  GnomeVFSFileInfoOptions info_options,
			  GnomeVFSDirectoryVisitOptions visit_options,
			  GnomeVFSDirectoryVisitFunc callback,
			  gpointer data)
{
	GnomeVFSFileInfo *info;
	GnomeVFSDirectoryHandle *handle;
	GnomeVFSResult result;
	gboolean stop;

	/* The first time, initialize the ancestor list with this
	   directory.  */
	if (prefix == NULL) {
		GnomeVFSFileInfo *info;

		info = gnome_vfs_file_info_new ();
		result = gnome_vfs_get_file_info_uri (uri, info,
						      info_options);
		if (result != GNOME_VFS_OK) {
			gnome_vfs_file_info_unref (info);
			return result;
		}

		if (info->type != GNOME_VFS_FILE_TYPE_DIRECTORY) {
			gnome_vfs_file_info_unref (info);
			return GNOME_VFS_ERROR_NOT_A_DIRECTORY;
		}

		ancestor_references = prepend_reference (ancestor_references,
							 info);
		gnome_vfs_file_info_unref (info);
	}

	result = gnome_vfs_directory_open_from_uri (&handle, uri, info_options);
	if (result != GNOME_VFS_OK)
		return result;

	info = gnome_vfs_file_info_new ();

	stop = FALSE;
	while (! stop) {
		gchar *rel_path;
		gboolean recurse;
		gboolean recursing_will_loop;

		result = gnome_vfs_directory_read_next (handle, info);
		if (result != GNOME_VFS_OK)
			break;

		/* Skip "." and "..".  */
		if (info->name[0] == '.'
		    && (info->name[1] == 0
			|| (info->name[1] == '.' && info->name[2] == 0))) {
			gnome_vfs_file_info_clear (info);
			continue;
		}

		if (prefix == NULL)
			rel_path = g_strdup (info->name);
		else
			rel_path = g_strconcat (prefix, info->name, NULL);

		if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY
		    && (visit_options & GNOME_VFS_DIRECTORY_VISIT_LOOPCHECK))
			recursing_will_loop
				= lookup_ancestor (ancestor_references,
						   (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_DEVICE) &&
						   (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_INODE),
						   info->inode, info->device);
		else
			recursing_will_loop = FALSE;

		recurse = FALSE;
		stop = ! (* callback) (rel_path, info, recursing_will_loop,
				       data, &recurse);
		if (stop)
			result = GNOME_VFS_ERROR_INTERRUPTED;

		if (! stop
		    && recurse
		    && info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
			GnomeVFSURI *new_uri;
			gchar *new_prefix;

			if (prefix == NULL)
				new_prefix = g_strconcat (info->name, "/",
							  NULL);
			else
				new_prefix = g_strconcat (prefix, info->name,
							  "/", NULL);

			new_uri = gnome_vfs_uri_append_file_name (uri, info->name);


			if (GNOME_VFS_FILE_INFO_LOCAL (info))
				ancestor_references = prepend_reference
					(ancestor_references, info);

			result = directory_visit_internal (new_uri,
							   new_prefix,
							   ancestor_references,
							   info_options,
							   visit_options,
							   callback, data);

			if (GNOME_VFS_FILE_INFO_LOCAL (info))
				ancestor_references = remove_first_reference
					(ancestor_references);

			if (result != GNOME_VFS_OK)
				stop = TRUE;

			gnome_vfs_uri_unref (new_uri);
			g_free (new_prefix);
		}

		g_free (rel_path);

		gnome_vfs_file_info_clear (info);

		if (stop)
			break;
	}

	gnome_vfs_directory_close (handle);
	gnome_vfs_file_info_unref (info);

	/* The first time, we are responsible for de-allocating the directory
           reference we have added by ourselves.  */
	if (prefix == NULL)
		ancestor_references
			= remove_first_reference (ancestor_references);

	if (result == GNOME_VFS_ERROR_EOF)
		return GNOME_VFS_OK;
	else
		return result;
}

/**
 * gnome_vfs_directory_visit_uri:
 * @uri: #GnomeVFSURI of a directory to visit the files in.
 * @info_options: #GnomeVFSFileInfoOptions specifying what kind of file information must be
 * retrieved about the contents of the directory referenced by @uri.
 * @visit_options: #GnomeVFSDirectoryVisitOptions controlling e.g. loop prevention,
 * and filesystem checks. These affect the way visiting is done.
 * @callback: #GnomeVFSDirectoryVisitFunc callback to be called for every visited file.
 * @data: data to be passed to @callback at each iteration.
 * 
 * Visit @uri, retrieving information as specified by @info_options. 
 *
 * This function is identical to gnome_vfs_directory_visit(), except
 * that it takes a #GnomeVFSURI instead of a text URI.
 * 
 * Returns: a #GnomeVFSResult indicating whether the operation succeeded.
 *
 */
GnomeVFSResult
gnome_vfs_directory_visit_uri (GnomeVFSURI *uri,
			       GnomeVFSFileInfoOptions info_options,
			       GnomeVFSDirectoryVisitOptions visit_options,
			       GnomeVFSDirectoryVisitFunc callback,
			       gpointer data)
{
	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (callback != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	return directory_visit_internal (uri, NULL, NULL,
					 info_options,
					 visit_options, callback, data);
}

/**
 * gnome_vfs_directory_visit:
 * @text_uri: uri string representation of a directory to visit the files in.
 * @info_options: #GnomeVFSFileInfoOptions specifying what kind of file information must be
 * retrieved about the contents of the directory referenced by @uri.
 * @visit_options: #GnomeVFSDirectoryVisitOptions controlling e.g. loop prevention,
 * and filesystem checks. These affect the way visiting is done.
 * @callback: #GnomeVFSDirectoryVisitFunc callback to be called for every visited file.
 * @data: data to be passed to @callback at each iteration.
 * 
 * Visit @text_uri, retrieving information as specified by @info_options.
 *
 * This function is identical to gnome_vfs_directory_visit_uri(), except
 * that it takes a text URI instead of a #GnomeVFSURI.
 * 
 * Return value: a #GnomeVFSResult indicating the success of the operation.
 */
GnomeVFSResult
gnome_vfs_directory_visit (const gchar *text_uri,
			   GnomeVFSFileInfoOptions info_options,
			   GnomeVFSDirectoryVisitOptions visit_options,
			   GnomeVFSDirectoryVisitFunc callback,
			   gpointer data)
{
	GnomeVFSURI *uri;
	GnomeVFSResult result;

	g_return_val_if_fail (text_uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (callback != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	uri = gnome_vfs_uri_new (text_uri);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	result = directory_visit_internal (uri, NULL, NULL,
					   info_options,
					   visit_options, callback, data);

	gnome_vfs_uri_unref (uri);

	return result;
}

/**
 * gnome_vfs_directory_visit_files_at_uri:
 * @uri: #GnomeVFSURI of a directory to visit the files in.
 * @file_list: #GList of char *s of file names in @uri to visit.
 * @info_options: #GnomeVFSFileInfoOptions specifying what kind of file information must be
 * retrieved about the contents of the directory referenced by @uri.
 * @visit_options: #GnomeVFSDirectoryVisitOptions controlling e.g. loop prevention,
 * and filesystem checks. These affect the way visiting is done.
 * @callback: #GnomeVFSDirectoryVisitFunc callback to call with the file info structs.
 * @data: data to pass to @callback.
 *
 * Fetches information about a list of files in a base uri @uri.
 *
 * This function is identical to gnome_vfs_directory_visit_files_at_uri(),
 * except that it takes a #GnomeVFSURI instead of a text URI.
 *
 * If any of the files refers to a directory, and the @callback requests
 * recursion for the specified file, gnome_vfs_directory_visit_uri() will
 * be called for the directory.
 *
 * Return value: a #GnomeVFSResult indicating the success of the operation.
 */
GnomeVFSResult
gnome_vfs_directory_visit_files_at_uri (GnomeVFSURI *uri,
					GList *file_list,
					GnomeVFSFileInfoOptions info_options,
					GnomeVFSDirectoryVisitOptions visit_options,
					GnomeVFSDirectoryVisitFunc callback,
					gpointer data)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	GList *p;

	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (file_list != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (callback != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	info = gnome_vfs_file_info_new ();
	result = GNOME_VFS_OK;

	for (p = file_list; p != NULL; p = p->next) {
		GnomeVFSURI *file_uri;
		gboolean recurse;
		gboolean stop;

		file_uri = gnome_vfs_uri_append_file_name (uri, p->data);
		gnome_vfs_get_file_info_uri (file_uri, 
					     info, 
					     info_options);

		recurse = FALSE;
		stop = ! (* callback) (info->name, info, FALSE, data,
				       &recurse);

		if (! stop
		    && recurse
		    && info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
			result = gnome_vfs_directory_visit_uri
				(uri, 
				 info_options,
				 visit_options,
				 callback, 
				 data);

		gnome_vfs_uri_unref (file_uri);

		if (result != GNOME_VFS_OK || stop)
			break;
	}

	gnome_vfs_file_info_unref (info);
	return GNOME_VFS_OK;
}

/**
 * gnome_vfs_directory_visit_files:
 * @text_uri: string representing the uri of a directory to visit the files in.
 * @file_list: #GList of char *s of file names in @uri to visit.
 * @info_options: bitmask controlling the type of information to fetch.
 * @visit_options: #GnomeVFSDirectoryVisitOptions controlling e.g. loop prevention,
 * and filesystem checks. These affect the way visiting is done.
 * @callback: #GnomeVFSDirectoryVisitFunc callback to call with the file info structs.
 * @data: data to pass to @callback.
 *
 * Fetches information about a list of files in a base uri @uri.
 *
 * If any of the files refers to a directory, and the @callback requests
 * recursion for the specified file, gnome_vfs_directory_visit_uri() will
 * be called for the directory.
 *
 * This function is identical to gnome_vfs_directory_visit_files_at_uri(),
 * except that it takes a text URI instead of a #GnomeVFSURI.
 *
 * Return value: a #GnomeVFSResult indicating the success of the operation.
 */
GnomeVFSResult
gnome_vfs_directory_visit_files (const gchar *text_uri,
				 GList *file_list,
				 GnomeVFSFileInfoOptions info_options,
				 GnomeVFSDirectoryVisitOptions visit_options,
				 GnomeVFSDirectoryVisitFunc callback,
				 gpointer data)
{
	GnomeVFSURI *uri;
	GnomeVFSResult result;

	uri = gnome_vfs_uri_new (text_uri);
	if (uri == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	result = gnome_vfs_directory_visit_files_at_uri (uri, file_list,
							 info_options,
							 visit_options,
							 callback,
							 data);
	gnome_vfs_uri_unref (uri);

	return result;
}

static GnomeVFSResult
load_from_handle (GList **list,
		  GnomeVFSDirectoryHandle *handle)
{
	GnomeVFSResult result;
	GnomeVFSFileInfo *info;

	*list = NULL;

	for (;;) {
		info = gnome_vfs_file_info_new ();
		result = gnome_vfs_directory_read_next (handle, info);
		if (result != GNOME_VFS_OK)
			break;
		*list = g_list_prepend (*list, info);
	}

	*list = g_list_reverse (*list);
	
	gnome_vfs_file_info_unref (info);

	if (result != GNOME_VFS_ERROR_EOF) {
		gnome_vfs_file_info_list_free (*list);
		*list = NULL;
	}

	return GNOME_VFS_OK;
}

/**
 * gnome_vfs_directory_list_load:
 * @list: address of a pointer to a list of #GnomeVFSFileInfo.
 * @text_uri: a string representing the uri of the directory.
 * @options: options for loading the directory.
 * 
 * Load a directory from @text_uri with the specified @options
 * into a list.
 * 
 * Return value: an integer representing the result of the operation.
 */
GnomeVFSResult 
gnome_vfs_directory_list_load (GList **list,
			       const gchar *text_uri,
			       GnomeVFSFileInfoOptions options)
{
	GnomeVFSDirectoryHandle *handle;
	GnomeVFSResult result;

	result = gnome_vfs_directory_open (&handle, text_uri, options);
	if (result != GNOME_VFS_OK) {
		return result;
	}

	result = load_from_handle (list, handle);

	gnome_vfs_directory_close (handle);
	return result;
}


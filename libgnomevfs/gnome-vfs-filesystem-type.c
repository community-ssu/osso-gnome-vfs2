/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-filesystem-type.c - Handling of filesystems for the GNOME Virtual File System.

   Copyright (C) 2003 Red Hat, Inc

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

   Author: Alexander Larsson <alexl@redhat.com>
*/

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include "gnome-vfs-filesystem-type.h"

struct FSInfo {
	char *fs_type;
	char *fs_name;
	gboolean use_trash;
};

/* NB: Keep in sync with gnome_vfs_volume_get_filesystem_type()
 * documentation! */
static struct FSInfo fs_data[] = {
	{ "affs"     , N_("AFFS Volume"), 0},
	{ "afs"      , N_("AFS Network Volume"), 0 },
	{ "auto"     , N_("Auto-detected Volume"), 0 },
	{ "cd9660"   , N_("CD-ROM Drive"), 0 },
	{ "cdda"     , N_("CD Digital Audio"), 0 },
	{ "cdrom"    , N_("CD-ROM Drive"), 0 },
	{ "devfs"    , N_("Hardware Device Volume"), 0 },
	{ "encfs"    , N_("EncFS Volume"), 1 },
	{ "ext2"     , N_("Ext2 Linux Volume"), 1 },
	{ "ext2fs"   , N_("Ext2 Linux Volume"), 1 },
	{ "ext3"     , N_("Ext3 Linux Volume"), 1 },
	{ "fat"      , N_("MSDOS Volume"), 1 },
	{ "ffs"      , N_("BSD Volume"), 1 },
	{ "hfs"	     , N_("MacOS Volume"), 1 },
	{ "hfsplus"  , N_("MacOS Volume"), 0 },
	{ "iso9660"  , N_("CDROM Volume"), 0 },
	{ "hsfs"     , N_("Hsfs CDROM Volume"), 0 },
	{ "jfs"      , N_("JFS Volume"), 1 },
	{ "hpfs"     , N_("Windows NT Volume"), 0 },
	{ "kernfs"   , N_("System Volume"), 0 },
	{ "lfs"      , N_("BSD Volume"), 1 },
	{ "linprocfs", N_("System Volume"), 0 },
	{ "mfs"      , N_("Memory Volume"), 1 },
	{ "minix"    , N_("Minix Volume"), 0 },
	{ "msdos"    , N_("MSDOS Volume"), 0 },
	{ "msdosfs"  , N_("MSDOS Volume"), 0 },
	{ "nfs"      , N_("NFS Network Volume"), 1 },
	{ "ntfs"     , N_("Windows NT Volume"), 0 },
	{ "nwfs"     , N_("Netware Volume"), 0 },
	{ "proc"     , N_("System Volume"), 0 },
	{ "procfs"   , N_("System Volume"), 0 },
	{ "ptyfs"    , N_("System Volume"), 0 },
	{ "reiser4"  , N_("Reiser4 Linux Volume"), 1 },
	{ "reiserfs" , N_("ReiserFS Linux Volume"), 1 },
	{ "smbfs"    , N_("Windows Shared Volume"), 1 },
	{ "supermount",N_("SuperMount Volume"), 0 },
	{ "udf"      , N_("DVD Volume"), 0 },
	{ "ufs"      , N_("Solaris/BSD Volume"), 1 },
	{ "udfs"     , N_("Udfs Solaris Volume"), 1 },
	{ "pcfs"     , N_("Pcfs Solaris Volume"), 1 },
	{ "samfs"    , N_("Sun SAM-QFS Volume"), 1 },
	{ "tmpfs"    , N_("Temporary Volume"), 1 },
	{ "umsdos"   , N_("Enhanced DOS Volume"), 0 },
	{ "vfat"     , N_("Windows VFAT Volume"), 1 },
	{ "xenix"    , N_("Xenix Volume"), 0 },
	{ "xfs"      , N_("XFS Linux Volume"), 1 },
	{ "xiafs"    , N_("XIAFS Volume"), 0 },
	{ "cifs"     , N_("CIFS Volume"), 1 },
};

static struct FSInfo *
find_fs_info (const char *fs_type)
{
	int i;

	for (i = 0; i < G_N_ELEMENTS (fs_data); i++) {
		if (strcmp (fs_data[i].fs_type, fs_type) == 0) {
			return &fs_data[i];
		}
	}
	
	return NULL;
}

char *
_gnome_vfs_filesystem_volume_name (const char *fs_type)
{
	struct FSInfo *info;

	info = find_fs_info (fs_type);

	if (info != NULL) {
		return g_strdup (_(info->fs_name));
	}
	
	return g_strdup (_("Unknown"));
}
     
gboolean
_gnome_vfs_filesystem_use_trash (const char *fs_type)
{
	struct FSInfo *info;

	info = find_fs_info (fs_type);

	if (info != NULL) {
		return info->use_trash;
	}
	
	return FALSE;
}

 /* Maemo patch */
#define MMC_LABEL_FILE                    "/tmp/.mmc-volume-label"
#define MMC_LABEL_FILE2                   "/tmp/.internal-mmc-volume-label"
#define MMC_LABEL_LENGTH                  11
#define MMC_LABEL_UNDEFINED_NAME          "mmc-undefined-name"
#define MMC_LABEL_UNDEFINED_NAME_INTERNAL "mmc-undefined-name-internal"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Same as make_utf8 but without adding "Invalid unicode". */
static char *
make_utf8_barebone (const char *name)
{
	GString    *string;
	const char *remainder, *invalid;
	int         remaining_bytes, valid_bytes;

	string = NULL;
	remainder = name;
	remaining_bytes = strlen (name);

	while (remaining_bytes != 0) {
		if (g_utf8_validate (remainder, remaining_bytes, &invalid)) {
			break;
		}
		valid_bytes = invalid - remainder;

		if (string == NULL) {
			string = g_string_sized_new (remaining_bytes);
		}
		g_string_append_len (string, remainder, valid_bytes);
		g_string_append_c (string, '?');

		remaining_bytes -= valid_bytes + 1;
		remainder = invalid + 1;
	}

	if (string == NULL) {
		return g_strdup (name);
	}

	g_string_append (string, remainder);

	return g_string_free (string, FALSE);
}

static char *
get_mmc_name (gboolean internal)
{
	const gchar *label_file;
	int          fd;
	ssize_t      size_read;
	char         buf[MMC_LABEL_LENGTH + 1];

	if (internal) {
		label_file = MMC_LABEL_FILE2;
	} else {
		label_file = MMC_LABEL_FILE;
	}
	
	if (!g_file_test (label_file, G_FILE_TEST_EXISTS)) {
		goto unknown;
	}

	fd = open (label_file, O_RDONLY);
	if (fd < 0) {
		goto unknown;
	}

	size_read = read (fd, buf, MMC_LABEL_LENGTH);
	close (fd);
	
	if (size_read > 0) {
		buf[size_read] = '\0';

		if (!g_utf8_validate (buf, size_read, NULL)) {
			return make_utf8_barebone (buf);
		}
		
		return g_strndup (buf, size_read);
	}

unknown:
	if (internal) {
		return g_strdup (MMC_LABEL_UNDEFINED_NAME_INTERNAL);
	} else {
		return g_strdup (MMC_LABEL_UNDEFINED_NAME);
	}		
}

char *
_gnome_vfs_filesystem_get_label_for_mount_point (const char *mount_point)
{
	const char *mmc_mount;

	if (!mount_point) {
		return NULL;
	}

	mmc_mount = g_getenv ("MMC_MOUNTPOINT");
	if (mmc_mount && g_str_has_prefix (mount_point, mmc_mount)) {
		return get_mmc_name (FALSE);
	}

       mmc_mount = g_getenv ("INTERNAL_MMC_MOUNTPOINT");
       if (mmc_mount && g_str_has_prefix (mount_point, mmc_mount)) {
               return get_mmc_name (TRUE);
       }

       return NULL;
}

char *
_gnome_vfs_filesystem_get_label_for_slot_name (const char *slot_name)
{
	if (!slot_name) {
		return NULL;
	}
	
	if (strcmp (slot_name, "external") == 0) {
		return get_mmc_name (FALSE);
	}
 
	if (strcmp (slot_name, "internal") == 0) {
		return get_mmc_name (TRUE);
	}
	
	return NULL;
}
/* End of maemo patch */

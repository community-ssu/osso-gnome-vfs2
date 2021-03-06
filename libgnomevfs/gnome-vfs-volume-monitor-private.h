/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-volume-monitor-private.h - Handling of volumes for the GNOME Virtual File System.

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

#ifndef GNOME_VFS_VOLUME_MONITOR_PRIVATE_H
#define GNOME_VFS_VOLUME_MONITOR_PRIVATE_H

#include <glib/gthread.h>
#include "gnome-vfs-volume-monitor.h"
#include <dbus/dbus.h>

#define CONNECTED_SERVERS_DIR "/desktop/gnome/connected_servers"

struct _GnomeVFSVolumeMonitorPrivate {
	GMutex *mutex;

	GList *fstab_drives;
	GList *vfs_drives;

	GList *mtab_volumes;
	GList *server_volumes;
	GList *vfs_volumes;
};

struct _GnomeVFSVolumePrivate {
	gulong id;
	GnomeVFSVolumeType volume_type;
	GnomeVFSDeviceType device_type;
	GnomeVFSDrive *drive; /* Non-owning ref */

	char *activation_uri;
	char *filesystem_type;
	char *display_name;
	char *display_name_key;
	char *icon;
	
	gboolean is_user_visible;
	gboolean is_read_only;
	gboolean is_mounted;

	/* Only for unix mounts: */
	char *device_path;
	dev_t unix_device;

	/* Only for HAL devices: */
	char *hal_udi;
	char *hal_drive_udi; /* only available to daemon; not exported */

	/* Only for connected servers */
	char *gconf_id;
};


struct _GnomeVFSDrivePrivate {
	gulong id;
	GnomeVFSDeviceType device_type;
	GList *volumes; /* GnomeVFSVolume list (Owning ref) */

	/* Only for unix mounts: */
	char *device_path;
	
	char *activation_uri;
	char *display_name;
	char *display_name_key;
	char *icon;
	
	gboolean is_user_visible;
	gboolean is_connected;

	/* Only for HAL devices: */
	char *hal_udi;
	char *hal_drive_udi; /* only available to daemon; not exported */
	char *hal_backing_crypto_volume_udi; /* only available to daemon; not exported */

	gboolean must_eject_at_unmount;
};

void gnome_vfs_volume_set_drive_private         (GnomeVFSVolume        *volume,
						 GnomeVFSDrive         *drive);
void gnome_vfs_drive_add_mounted_volume_private (GnomeVFSDrive         *drive,
						 GnomeVFSVolume        *volume);
void gnome_vfs_drive_remove_volume_private      (GnomeVFSDrive         *drive,
						 GnomeVFSVolume        *volume);
void gnome_vfs_volume_unset_drive_private       (GnomeVFSVolume        *volume,
						 GnomeVFSDrive         *drive);
void _gnome_vfs_volume_monitor_mounted          (GnomeVFSVolumeMonitor *volume_monitor,
						 GnomeVFSVolume        *volume);
void _gnome_vfs_volume_monitor_unmounted        (GnomeVFSVolumeMonitor *volume_monitor,
						 GnomeVFSVolume        *volume);
void _gnome_vfs_volume_monitor_connected        (GnomeVFSVolumeMonitor *volume_monitor,
						 GnomeVFSDrive         *drive);
void _gnome_vfs_volume_monitor_disconnected     (GnomeVFSVolumeMonitor *volume_monitor,
						 GnomeVFSDrive         *drive);
void _gnome_vfs_volume_monitor_disconnect_all   (GnomeVFSVolumeMonitor *volume_monitor);
void _gnome_vfs_volume_monitor_unmount_all      (GnomeVFSVolumeMonitor *volume_monitor);
void gnome_vfs_volume_monitor_emit_pre_unmount (GnomeVFSVolumeMonitor *volume_monitor,
						GnomeVFSVolume        *volume);
void _gnome_vfs_volume_monitor_force_probe (GnomeVFSVolumeMonitor *volume_monitor);

GnomeVFSVolumeMonitor *_gnome_vfs_get_volume_monitor_internal (gboolean create);
void _gnome_vfs_volume_monitor_shutdown (void);

int _gnome_vfs_device_type_get_sort_group (GnomeVFSDeviceType type);

GnomeVFSVolume *_gnome_vfs_volume_from_dbus (DBusMessageIter       *iter,
					     GnomeVFSVolumeMonitor *volume_monitor);
gboolean        gnome_vfs_volume_to_dbus    (DBusMessageIter       *iter,
					     GnomeVFSVolume        *volume);
gboolean        gnome_vfs_drive_to_dbus     (DBusMessageIter       *iter,
					     GnomeVFSDrive         *drive);
GnomeVFSDrive * _gnome_vfs_drive_from_dbus  (DBusMessageIter       *iter,
					     GnomeVFSVolumeMonitor *volume_monitor);

GnomeVFSVolume *_gnome_vfs_volume_monitor_find_mtab_volume_by_activation_uri (GnomeVFSVolumeMonitor *volume_monitor,
									      const char            *activation_uri);
GnomeVFSDrive * _gnome_vfs_volume_monitor_find_fstab_drive_by_activation_uri (GnomeVFSVolumeMonitor *volume_monitor,
									      const char            *activation_uri);
GnomeVFSVolume *_gnome_vfs_volume_monitor_find_connected_server_by_gconf_id  (GnomeVFSVolumeMonitor *volume_monitor,
									      const char            *id);

#ifdef USE_HAL
GnomeVFSVolume *_gnome_vfs_volume_monitor_find_volume_by_hal_udi (GnomeVFSVolumeMonitor *volume_monitor,
								  const char            *hal_udi);
GnomeVFSDrive *_gnome_vfs_volume_monitor_find_drive_by_hal_udi   (GnomeVFSVolumeMonitor *volume_monitor,
								  const char            *hal_udi);

GnomeVFSVolume *_gnome_vfs_volume_monitor_find_volume_by_hal_drive_udi (GnomeVFSVolumeMonitor *volume_monitor,
									const char            *hal_drive_udi);
GnomeVFSDrive *_gnome_vfs_volume_monitor_find_drive_by_hal_drive_udi   (GnomeVFSVolumeMonitor *volume_monitor,
									const char            *hal_drive_udi);

#endif /* USE_HAL */

GnomeVFSVolume *_gnome_vfs_volume_monitor_find_volume_by_device_path (GnomeVFSVolumeMonitor *volume_monitor,
								      const char            *device_path);
GnomeVFSDrive *_gnome_vfs_volume_monitor_find_drive_by_device_path   (GnomeVFSVolumeMonitor *volume_monitor,
								      const char            *device_path);



char *_gnome_vfs_volume_monitor_uniquify_volume_name (GnomeVFSVolumeMonitor *volume_monitor,
						      const char            *name);
char *_gnome_vfs_volume_monitor_uniquify_drive_name  (GnomeVFSVolumeMonitor *volume_monitor,
						      const char            *name);



#endif /* GNOME_VFS_VOLUME_MONITOR_PRIVATE_H */

/*
 * ROX-Filer GTK3 partition toolbar integration.
 *
 * Agregado por josejp2424 (2026): detección de particiones inspirada en la
 * integración de unidades de EssoraWM, con montaje directo para Puppy/root,
 * alternativa mediante udisksctl para usuarios normales y apertura de la
 * partición dentro de la ventana actual de ROX-Filer.
 *
 * Copyright (C) 2026 josejp2424
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 */

#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "global.h"
#include "drives.h"
#include "filer.h"
#include "gui_support.h"

#define DRIVE_ICON_INTERNAL  "drive-harddisk"
#define DRIVE_ICON_REMOVABLE "drive-removable-media-usb"

typedef struct _DriveInfo DriveInfo;

struct _DriveInfo
{
	gchar *name;
	gchar *device;
	gchar *label;
	gchar *fstype;
	gchar *mountpoint;
	gchar *size;
	gboolean removable;
};

typedef struct
{
	FilerWindow *filer_window;
	DriveInfo *drive;
	GtkWidget *popover;
} DriveMenuAction;

static gchar *parse_lsblk_value(const gchar *line, const gchar *key);
static GPtrArray *read_drive_list(GError **error);
static gboolean drive_is_useful(const DriveInfo *drive, const gchar *type,
		const gchar *partlabel, const gchar *parttype);
static gboolean drive_array_has_device(GPtrArray *drives, const gchar *device);
static gboolean drive_is_hidden_by_essorawm(const gchar *name);
static gchar *command_first_line(gchar **argv);
static DriveInfo *drive_info_from_device(const gchar *device);
static void append_puppy_runtime_drives(GPtrArray *drives);
static void append_sysfs_partitions(GPtrArray *drives);
static gboolean technical_text_match(const gchar *value);
static gboolean name_looks_removable(const gchar *value);
static void drive_info_free(gpointer data);
static gchar *find_mountpoint(const gchar *device);
static gboolean spawn_wait(gchar **argv, gchar **error_text);
static gchar *mount_drive(const DriveInfo *drive, gchar **error_text);
static void drive_menu_action_free(gpointer data);
static void drive_grid_activate(GtkButton *button, gpointer data);
static void drives_button_clicked(GtkToolButton *button, gpointer data);
static GtkWidget *drive_grid_button_new(const DriveInfo *drive);
static GtkWidget *drive_icon_widget(const DriveInfo *drive, gint size);

static gint hex_value(gchar value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

/* Agregado por josejp2424: decodificar de forma segura el formato -P de
 * lsblk, incluidos espacios expresados como secuencias \xNN. */
static gchar *parse_lsblk_value(const gchar *line, const gchar *key)
{
	gchar *pattern;
	const gchar *start;
	const gchar *end;
	GString *output;

	pattern = g_strdup_printf("%s=\"", key);
	start = strstr(line, pattern);
	g_free(pattern);
	if (!start)
		return NULL;

	start = strchr(start, '"');
	if (!start)
		return NULL;
	start++;
	end = start;
	while (*end)
	{
		if (*end == '"' && (end == start || end[-1] != '\\'))
			break;
		end++;
	}

	output = g_string_new(NULL);
	while (start < end)
	{
		if (*start == '\\' && start + 3 < end && start[1] == 'x')
		{
			gint high = hex_value(start[2]);
			gint low = hex_value(start[3]);
			if (high >= 0 && low >= 0)
			{
				g_string_append_c(output, (gchar) ((high << 4) | low));
				start += 4;
				continue;
			}
		}
		if (*start == '\\' && start + 1 < end)
			start++;
		g_string_append_c(output, *start++);
	}

	return g_string_free(output, FALSE);
}

static gboolean name_looks_removable(const gchar *value)
{
	gchar *lower;
	gboolean result;

	if (!value || !*value)
		return FALSE;

	lower = g_utf8_strdown(value, -1);
	result = strstr(lower, "usb") != NULL ||
		strstr(lower, "ventoy") != NULL ||
		strstr(lower, "pendrive") != NULL ||
		strstr(lower, "flash") != NULL ||
		strstr(lower, "removable") != NULL ||
		strstr(lower, "sd card") != NULL ||
		strstr(lower, "memory card") != NULL;
	g_free(lower);
	return result;
}

static gboolean technical_text_match(const gchar *value)
{
	gchar *lower;
	gchar *compact;
	const gchar *read;
	gchar *write;
	gboolean result;

	if (!value || !*value)
		return FALSE;

	lower = g_utf8_strdown(value, -1);
	compact = g_malloc(strlen(lower) + 1);
	write = compact;
	for (read = lower; *read; read++)
	{
		if (g_ascii_isalnum((guchar) *read))
			*write++ = *read;
	}
	*write = '\0';

	result = strstr(compact, "pupro") || strstr(compact, "puprw") ||
		!strcmp(compact, "pupa") || !strcmp(compact, "pupb") ||
		!strcmp(compact, "pupf") || !strcmp(compact, "pupk") ||
		!strcmp(compact, "pupz") || strstr(compact, "vtoyefi") ||
		strstr(lower, "/boot/efi") || strstr(lower, "efi system") ||
		!strcmp(compact, "efi") || !strcmp(compact, "esp") ||
		!strcmp(compact, "efisystempartition") ||
		!strcmp(compact, "swap");

	g_free(compact);
	g_free(lower);
	return result;
}

static gboolean drive_is_useful(const DriveInfo *drive, const gchar *type,
		const gchar *partlabel, const gchar *parttype)
{
	const gchar *base;
	gboolean useful_type;

	if (!drive || !drive->device || strncmp(drive->device, "/dev/", 5))
		return FALSE;

	base = strrchr(drive->device, '/');
	base = base ? base + 1 : drive->device;
	if (!g_ascii_strncasecmp(base, "loop", 4) ||
	    !g_ascii_strncasecmp(base, "zram", 4) ||
	    !g_ascii_strncasecmp(base, "ram", 3) ||
	    !g_ascii_strncasecmp(base, "dm-", 3))
		return FALSE;

	if (drive->mountpoint &&
	    (!strncmp(drive->mountpoint, "/initrd/pup_", 12) ||
	     !strncmp(drive->mountpoint, "/pup_", 5)))
		return FALSE;

	if (drive->fstype &&
	    (!g_ascii_strcasecmp(drive->fstype, "swap") ||
	     !g_ascii_strcasecmp(drive->fstype, "squashfs") ||
	     !g_ascii_strcasecmp(drive->fstype, "overlay") ||
	     !g_ascii_strcasecmp(drive->fstype, "aufs")))
		return FALSE;

	if (parttype &&
	    (!g_ascii_strcasecmp(parttype,
		"c12a7328-f81f-11d2-ba4b-00a0c93ec93b") ||
	     !g_ascii_strcasecmp(parttype, "ef00")))
		return FALSE;

	if (technical_text_match(drive->label) ||
	    technical_text_match(drive->mountpoint) ||
	    technical_text_match(partlabel))
		return FALSE;

	useful_type = type && (!strcmp(type, "part") || !strcmp(type, "crypt") ||
		!strcmp(type, "lvm") || !strcmp(type, "rom"));

	/* Modificado por josejp2424 (2026): usar exactamente la condición de
	 * EssoraWM. Una entrada debe ser una partición/volumen útil y, además,
	 * tener sistema de archivos, estar montada o ser removible. Esto evita
	 * mostrar discos físicos, particiones técnicas y entradas vacías de sysfs. */
	return drive->name && *drive->name &&
		(useful_type || (drive->fstype && *drive->fstype) ||
		 (drive->mountpoint && *drive->mountpoint)) &&
		((drive->fstype && *drive->fstype) ||
		 (drive->mountpoint && *drive->mountpoint) || drive->removable) &&
		!drive_is_hidden_by_essorawm(drive->name);
}

/* Agregado por josejp2424 (2026): evitar duplicados al combinar lsblk,
 * los iconos runtime de Puppy y /sys/class/block. */
static gboolean drive_array_has_device(GPtrArray *drives, const gchar *device)
{
	guint i;

	if (!drives || !device)
		return FALSE;
	for (i = 0; i < drives->len; i++)
	{
		DriveInfo *drive = g_ptr_array_index(drives, i);
		if (drive->device && !strcmp(drive->device, device))
			return TRUE;
	}
	return FALSE;
}

/* Agregado por josejp2424 (2026): respetar la misma lista de unidades
 * ocultas que EssoraWM. Cada línea contiene el nombre corto del dispositivo
 * (por ejemplo sda1 o nvme0n1p2). */
static gboolean drive_is_hidden_by_essorawm(const gchar *name)
{
	gchar *path;
	gchar *contents = NULL;
	gchar **lines;
	gboolean hidden = FALSE;
	gint i;

	if (!name || !*name)
		return FALSE;

	path = g_build_filename(g_get_home_dir(), ".config", "essorawm",
		"hidden-drives", NULL);
	if (!g_file_get_contents(path, &contents, NULL, NULL))
	{
		g_free(path);
		return FALSE;
	}
	g_free(path);

	lines = g_strsplit(contents, "\n", -1);
	for (i = 0; lines[i]; i++)
	{
		gchar *line = g_strstrip(lines[i]);
		if (*line && !strcmp(line, name))
		{
			hidden = TRUE;
			break;
		}
	}
	g_strfreev(lines);
	g_free(contents);
	return hidden;
}

/* Agregado por josejp2424 (2026): ejecutar una consulta pequeña y devolver
 * solamente la primera línea sin espacios finales. */
static gchar *command_first_line(gchar **argv)
{
	gchar *output = NULL;
	gchar *error_output = NULL;
	gint status = 0;
	GError *error = NULL;
	gchar *line;

	if (!g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
		&output, &error_output, &status, &error))
	{
		g_clear_error(&error);
		g_free(output);
		g_free(error_output);
		return NULL;
	}
	g_free(error_output);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || !output)
	{
		g_free(output);
		return NULL;
	}

	line = g_strstrip(output);
	if (!*line)
	{
		g_free(output);
		return NULL;
	}
	return output;
}

/* Agregado por josejp2424 (2026): construir metadatos para una partición
 * detectada fuera de lsblk. Se consultan blkid y lsblk de forma opcional. */
static DriveInfo *drive_info_from_device(const gchar *device)
{
	DriveInfo *drive;
	gchar *base;
	gchar *argv_type[] = {(gchar *) "blkid", (gchar *) "-o",
		(gchar *) "value", (gchar *) "-s", (gchar *) "TYPE",
		(gchar *) device, NULL};
	gchar *argv_label[] = {(gchar *) "blkid", (gchar *) "-o",
		(gchar *) "value", (gchar *) "-s", (gchar *) "LABEL",
		(gchar *) device, NULL};
	gchar *argv_size[] = {(gchar *) "lsblk", (gchar *) "-dn",
		(gchar *) "-o", (gchar *) "SIZE", (gchar *) device, NULL};

	if (!device || strncmp(device, "/dev/", 5))
		return NULL;

	drive = g_new0(DriveInfo, 1);
	base = g_path_get_basename(device);
	drive->name = g_strdup(base);
	drive->device = g_strdup(device);
	drive->mountpoint = find_mountpoint(device);
	drive->fstype = command_first_line(argv_type);
	drive->label = command_first_line(argv_label);
	drive->size = command_first_line(argv_size);

	{
		gchar *removable_path = g_build_filename("/sys/class/block", base,
			"removable", NULL);
		gchar *value = NULL;
		if (g_file_get_contents(removable_path, &value, NULL, NULL))
			drive->removable = atoi(value) != 0;
		g_free(value);
		g_free(removable_path);
	}

	if (!drive->label || !*drive->label)
	{
		g_free(drive->label);
		drive->label = drive->size && *drive->size
			? g_strdup_printf(_("Volume %s"), drive->size)
			: g_strdup(base);
	}
	g_free(base);
	return drive;
}

/* Agregado por josejp2424 (2026): Puppy crea archivos drive_* para las
 * unidades que muestra en el escritorio. Usarlos como fuente adicional hace
 * que el botón de ROX vea exactamente las mismas particiones disponibles. */
static void append_puppy_runtime_drives(GPtrArray *drives)
{
	GDir *dir;
	const gchar *name;

	dir = g_dir_open("/tmp/pup_event_frontend", 0, NULL);
	if (!dir)
		return;

	while ((name = g_dir_read_name(dir)) != NULL)
	{
		gchar *device;
		DriveInfo *drive;

		if (!g_str_has_prefix(name, "drive_") || !name[6])
			continue;
		device = g_build_filename("/dev", name + 6, NULL);
		if (drive_array_has_device(drives, device))
		{
			g_free(device);
			continue;
		}
		drive = drive_info_from_device(device);
		if (drive && drive_is_useful(drive, "part", NULL, NULL))
			g_ptr_array_add(drives, drive);
		else
			drive_info_free(drive);
		g_free(device);
	}
	g_dir_close(dir);
}

/* Agregado por josejp2424 (2026): último respaldo sin depender de columnas
 * particulares de lsblk. /sys/class/block/<nombre>/partition identifica
 * particiones reales en discos SATA, NVMe, MMC y USB. */
static void append_sysfs_partitions(GPtrArray *drives)
{
	GDir *dir;
	const gchar *name;

	dir = g_dir_open("/sys/class/block", 0, NULL);
	if (!dir)
		return;

	while ((name = g_dir_read_name(dir)) != NULL)
	{
		gchar *partition_flag = g_build_filename("/sys/class/block", name,
			"partition", NULL);
		gchar *device;
		DriveInfo *drive;

		if (!g_file_test(partition_flag, G_FILE_TEST_EXISTS))
		{
			g_free(partition_flag);
			continue;
		}
		g_free(partition_flag);

		device = g_build_filename("/dev", name, NULL);
		if (drive_array_has_device(drives, device))
		{
			g_free(device);
			continue;
		}
		drive = drive_info_from_device(device);
		if (drive && drive_is_useful(drive, "part", NULL, NULL))
			g_ptr_array_add(drives, drive);
		else
			drive_info_free(drive);
		g_free(device);
	}
	g_dir_close(dir);
}

static void drive_info_free(gpointer data)
{
	DriveInfo *drive = data;
	if (!drive)
		return;
	g_free(drive->name);
	g_free(drive->device);
	g_free(drive->label);
	g_free(drive->fstype);
	g_free(drive->mountpoint);
	g_free(drive->size);
	g_free(drive);
}

/* Agregado por josejp2424: usar la misma idea de EssoraWM para mostrar sólo
 * particiones reales y evitar capas técnicas de Puppy, swap, loop y EFI. */
static GPtrArray *read_drive_list(GError **error)
{
	gchar *stdout_text = NULL;
	gchar *stderr_text = NULL;
	gint status = 0;
	gchar *argv_full[] = {
		(gchar *) "lsblk", (gchar *) "-P", (gchar *) "-p",
		(gchar *) "-o",
		(gchar *) "NAME,PATH,LABEL,FSTYPE,MOUNTPOINT,RM,TYPE,HOTPLUG,TRAN,MODEL,PARTLABEL,PARTTYPE,SIZE",
		NULL
	};
	gchar *argv_compat[] = {
		(gchar *) "lsblk", (gchar *) "-P", (gchar *) "-p",
		(gchar *) "-o",
		(gchar *) "NAME,LABEL,FSTYPE,MOUNTPOINT,RM,TYPE,TRAN,MODEL,SIZE",
		NULL
	};
	gchar **lines;
	gint i;
	GPtrArray *drives;
	GError *spawn_error = NULL;

	drives = g_ptr_array_new_with_free_func(drive_info_free);

	/* Modificado por josejp2424: algunos Puppy incluyen una versión antigua
	 * de lsblk sin PATH, HOTPLUG, PARTLABEL o PARTTYPE. Intentar primero la
	 * consulta completa de EssoraWM y repetir con columnas compatibles. */
	if (!g_spawn_sync(NULL, argv_full, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
		&stdout_text, &stderr_text, &status, &spawn_error) ||
	    !WIFEXITED(status) || WEXITSTATUS(status) != 0)
	{
		g_clear_error(&spawn_error);
		g_clear_pointer(&stdout_text, g_free);
		g_clear_pointer(&stderr_text, g_free);
		status = 0;
		if (!g_spawn_sync(NULL, argv_compat, NULL, G_SPAWN_SEARCH_PATH,
			NULL, NULL, &stdout_text, &stderr_text, &status, &spawn_error))
		{
			/* Modificado por josejp2424 (2026): no abandonar la detección
			 * cuando lsblk no está disponible. Puppy y sysfs siguen siendo
			 * fuentes válidas para listar las unidades del escritorio. */
			g_clear_error(&spawn_error);
			g_free(stderr_text);
			append_puppy_runtime_drives(drives);
			append_sysfs_partitions(drives);
			return drives;
		}
	}

	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
	{
		/* Modificado por josejp2424 (2026): usar los respaldos locales
		 * antes de informar un error. */
		g_free(stdout_text);
		g_free(stderr_text);
		append_puppy_runtime_drives(drives);
		append_sysfs_partitions(drives);
		if (drives->len == 0)
			g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
				"%s", _("No usable partitions found"));
		return drives;
	}
	g_free(stderr_text);

	lines = g_strsplit(stdout_text ? stdout_text : "", "\n", -1);
	for (i = 0; lines[i]; i++)
	{
		DriveInfo *drive;
		gchar *rm;
		gchar *hotplug;
		gchar *type;
		gchar *tran;
		gchar *model;
		gchar *partlabel;
		gchar *parttype;

		if (!*lines[i])
			continue;

		drive = g_new0(DriveInfo, 1);
		drive->name = parse_lsblk_value(lines[i], "NAME");
		drive->device = parse_lsblk_value(lines[i], "PATH");
		if (!drive->device && drive->name && drive->name[0] == '/')
			drive->device = g_strdup(drive->name);
		else if (!drive->device && drive->name && *drive->name)
			drive->device = g_build_filename("/dev", drive->name, NULL);
		if (drive->name)
		{
			gchar *base = g_path_get_basename(drive->name);
			g_free(drive->name);
			drive->name = base;
		}
		drive->label = parse_lsblk_value(lines[i], "LABEL");
		drive->fstype = parse_lsblk_value(lines[i], "FSTYPE");
		drive->mountpoint = parse_lsblk_value(lines[i], "MOUNTPOINT");
		drive->size = parse_lsblk_value(lines[i], "SIZE");
		rm = parse_lsblk_value(lines[i], "RM");
		hotplug = parse_lsblk_value(lines[i], "HOTPLUG");
		type = parse_lsblk_value(lines[i], "TYPE");
		tran = parse_lsblk_value(lines[i], "TRAN");
		model = parse_lsblk_value(lines[i], "MODEL");
		partlabel = parse_lsblk_value(lines[i], "PARTLABEL");
		parttype = parse_lsblk_value(lines[i], "PARTTYPE");

		drive->removable = (rm && atoi(rm) != 0) ||
			(hotplug && atoi(hotplug) != 0) ||
			(tran && (!g_ascii_strcasecmp(tran, "usb") ||
			          !g_ascii_strcasecmp(tran, "mmc") ||
			          !g_ascii_strcasecmp(tran, "sd"))) ||
			name_looks_removable(drive->label) ||
			name_looks_removable(partlabel) ||
			name_looks_removable(model);

		if (drive_is_useful(drive, type, partlabel, parttype))
		{
			if ((!drive->label || !*drive->label) && partlabel && *partlabel)
			{
				g_free(drive->label);
				drive->label = g_strdup(partlabel);
			}
			else if ((!drive->label || !*drive->label) &&
			         drive->size && *drive->size)
			{
				g_free(drive->label);
				drive->label = g_strdup_printf(_("Volume %s"), drive->size);
			}
			g_ptr_array_add(drives, drive);
		}
		else
			drive_info_free(drive);

		g_free(rm);
		g_free(hotplug);
		g_free(type);
		g_free(tran);
		g_free(model);
		g_free(partlabel);
		g_free(parttype);
	}

	g_strfreev(lines);
	g_free(stdout_text);

	/* Modificado por josejp2424 (2026): cuando lsblk funciona, conservar
	 * exclusivamente su lista filtrada con las mismas reglas de EssoraWM.
	 * Los respaldos de Puppy/sysfs sólo se usan si lsblk falla por completo;
	 * mezclarlos aquí agregaba particiones técnicas o sin uso visible. */
	return drives;
}

static gchar *find_mountpoint(const gchar *device)
{
	FILE *mounts;
	struct mntent *entry;
	gchar *device_real;
	gchar *result = NULL;

	if (!device || !*device)
		return NULL;

	device_real = realpath(device, NULL);
	mounts = setmntent("/proc/self/mounts", "r");
	if (!mounts)
	{
		free(device_real);
		return NULL;
	}

	while ((entry = getmntent(mounts)) != NULL)
	{
		gchar *entry_real = realpath(entry->mnt_fsname, NULL);
		gboolean same = !strcmp(entry->mnt_fsname, device) ||
			(device_real && entry_real && !strcmp(device_real, entry_real));
		free(entry_real);
		if (same)
		{
			result = g_strdup(entry->mnt_dir);
			break;
		}
	}

	endmntent(mounts);
	free(device_real);
	return result;
}

static gboolean spawn_wait(gchar **argv, gchar **error_text)
{
	gchar *stderr_text = NULL;
	gint status = 0;
	GError *error = NULL;
	gboolean ok;

	if (error_text)
		*error_text = NULL;

	ok = g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
		NULL, &stderr_text, &status, &error);
	if (!ok)
	{
		if (error_text)
			*error_text = g_strdup(error->message);
		g_error_free(error);
		g_free(stderr_text);
		return FALSE;
	}

	ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
	if (!ok && error_text)
		*error_text = g_strdup(stderr_text && *stderr_text ? stderr_text :
			_("The mount command failed."));
	g_free(stderr_text);
	return ok;
}

/* Agregado por josejp2424: Puppy/root monta directamente en /mnt/<dispositivo>;
 * otros usuarios utilizan udisksctl cuando está disponible. */
static gchar *mount_drive(const DriveInfo *drive, gchar **error_text)
{
	gchar *mountpoint;
	gchar *udisksctl;
	gchar *local_error = NULL;

	if (error_text)
		*error_text = NULL;
	if (!drive || !drive->device)
		return NULL;

	mountpoint = find_mountpoint(drive->device);
	if (mountpoint)
		return mountpoint;

	if (geteuid() == 0)
	{
		gchar *base = g_path_get_basename(drive->device);
		gchar *target = g_build_filename("/mnt", base, NULL);
		gboolean existed = g_file_test(target, G_FILE_TEST_IS_DIR);
		gchar *argv[] = {(gchar *) "/bin/mount", drive->device, target, NULL};

		g_free(base);
		if (g_mkdir_with_parents(target, 0755) == 0 &&
		    spawn_wait(argv, &local_error))
		{
			g_free(local_error);
			return target;
		}
		if (!existed)
			rmdir(target);
		g_free(target);
	}

	udisksctl = g_find_program_in_path("udisksctl");
	if (udisksctl)
	{
		gchar *argv[] = {udisksctl, (gchar *) "mount", (gchar *) "-b",
			drive->device, NULL};
		g_free(local_error);
		local_error = NULL;
		if (spawn_wait(argv, &local_error))
		{
			g_free(udisksctl);
			g_free(local_error);
			return find_mountpoint(drive->device);
		}
		g_free(udisksctl);
	}

	if (error_text)
		*error_text = local_error ? local_error :
			g_strdup_printf(_("Could not mount '%s'."), drive->device);
	else
		g_free(local_error);
	return NULL;
}

static void drive_menu_action_free(gpointer data)
{
	DriveMenuAction *action = data;
	if (!action)
		return;
	drive_info_free(action->drive);
	g_free(action);
}

static DriveInfo *drive_info_copy(const DriveInfo *source)
{
	DriveInfo *copy;
	if (!source)
		return NULL;
	copy = g_new0(DriveInfo, 1);
	copy->name = g_strdup(source->name);
	copy->device = g_strdup(source->device);
	copy->label = g_strdup(source->label);
	copy->fstype = g_strdup(source->fstype);
	copy->mountpoint = g_strdup(source->mountpoint);
	copy->size = g_strdup(source->size);
	copy->removable = source->removable;
	return copy;
}

static void drive_grid_activate(GtkButton *button, gpointer data)
{
	DriveMenuAction *action = data;
	GtkWidget *popover;
	gchar *mountpoint;
	gchar *error_text = NULL;

	(void) button;
	if (!action || !action->drive || !action->filer_window ||
	    !filer_exists(action->filer_window))
		return;

	popover = action->popover;
	mountpoint = find_mountpoint(action->drive->device);
	if (!mountpoint && action->drive->mountpoint &&
	    *action->drive->mountpoint &&
	    g_file_test(action->drive->mountpoint, G_FILE_TEST_IS_DIR))
		mountpoint = g_strdup(action->drive->mountpoint);

	if (!mountpoint)
		mountpoint = mount_drive(action->drive, &error_text);

	if (!mountpoint)
	{
		report_error("%s", error_text ? error_text :
			_("The partition could not be mounted."));
		g_free(error_text);
		return;
	}

	if (!g_file_test(mountpoint, G_FILE_TEST_IS_DIR))
	{
		report_error(_("The partition '%s' was mounted, but its mount point could not be found."),
			action->drive->device);
		g_free(mountpoint);
		g_free(error_text);
		return;
	}

	{
		FilerWindow *target_window = action->filer_window;

		/* Modificado por josejp2424 (2026): cerrar primero la cuadrícula.
		 * Al destruirla se libera la acción del botón, por eso conservamos
		 * previamente el puntero de la ventana que debe abrir la unidad. */
		if (GTK_IS_WIDGET(popover))
			gtk_widget_destroy(popover);
		filer_change_to(target_window, mountpoint, NULL);
	}
	g_free(mountpoint);
	g_free(error_text);
}

static GtkWidget *drive_icon_widget(const DriveInfo *drive, gint size)
{
	const gchar *icon_name;
	GtkWidget *image;

	icon_name = drive && drive->removable ? DRIVE_ICON_REMOVABLE :
		DRIVE_ICON_INTERNAL;
	image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_DIALOG);
	gtk_image_set_pixel_size(GTK_IMAGE(image), size);
	return image;
}

/* Agregado por josejp2424 (2026): representar cada partición como un botón
 * compacto para poder distribuir las unidades horizontalmente en grupos de
 * cuatro, en lugar de una única lista vertical o una hilera interminable. */
static GtkWidget *drive_grid_button_new(const DriveInfo *drive)
{
	GtkWidget *button;
	GtkWidget *box;
	GtkWidget *image;
	GtkWidget *label_widget;
	gchar *mountpoint;
	gchar *detail;
	gchar *markup;
	const gchar *title;
	const gchar *status;

	mountpoint = find_mountpoint(drive->device);
	status = mountpoint ? _("Mounted") : _("Not mounted");
	title = drive->label && *drive->label ? drive->label :
		(drive->name && *drive->name ? drive->name : drive->device);

	if (drive->size && *drive->size)
		detail = g_strdup_printf("%s · %s",
			drive->name ? drive->name : drive->device, drive->size);
	else
		detail = g_strdup(drive->name ? drive->name : drive->device);

	markup = g_markup_printf_escaped("<b>%s</b>\n%s\n%s",
		title ? title : _("Partitions"), detail ? detail : "", status);

	button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_widget_set_size_request(button, 138, 96);
	gtk_widget_set_hexpand(button, TRUE);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_widget_set_margin_start(box, 5);
	gtk_widget_set_margin_end(box, 5);
	gtk_widget_set_margin_top(box, 5);
	gtk_widget_set_margin_bottom(box, 5);
	gtk_container_add(GTK_CONTAINER(button), box);

	image = drive_icon_widget(drive, 32);
	gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);

	label_widget = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label_widget), markup);
	gtk_label_set_justify(GTK_LABEL(label_widget), GTK_JUSTIFY_CENTER);
	gtk_label_set_ellipsize(GTK_LABEL(label_widget), PANGO_ELLIPSIZE_END);
	gtk_label_set_max_width_chars(GTK_LABEL(label_widget), 20);
	gtk_widget_set_tooltip_text(button, drive->device);
	gtk_box_pack_start(GTK_BOX(box), label_widget, TRUE, TRUE, 0);

	g_free(markup);
	g_free(detail);
	g_free(mountpoint);
	return button;
}

static void destroy_popover_when_closed(GtkPopover *popover, gpointer data)
{
	(void) data;
	gtk_widget_destroy(GTK_WIDGET(popover));
}

static void drives_button_clicked(GtkToolButton *button, gpointer data)
{
	FilerWindow *filer_window = data;
	GtkWidget *popover;
	GtkWidget *outer;
	GtkWidget *title;
	GtkWidget *scrolled;
	GtkWidget *grid;
	GPtrArray *drives;
	GError *error = NULL;
	guint i;
	guint rows;
	gint content_height;

	/* Agregado por josejp2424 (2026): GtkPopover con GtkGrid de cuatro
	 * columnas. La quinta unidad comienza una nueva fila y así sucesivamente. */
	popover = gtk_popover_new(GTK_WIDGET(button));
	gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);
	gtk_popover_set_modal(GTK_POPOVER(popover), TRUE);
	g_signal_connect(popover, "closed",
		G_CALLBACK(destroy_popover_when_closed), NULL);

	outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_widget_set_margin_start(outer, 10);
	gtk_widget_set_margin_end(outer, 10);
	gtk_widget_set_margin_top(outer, 10);
	gtk_widget_set_margin_bottom(outer, 10);
	gtk_container_add(GTK_CONTAINER(popover), outer);

	title = gtk_label_new(NULL);
	{
		gchar *title_markup = g_markup_printf_escaped("<b>%s</b>",
			_("Partitions"));
		gtk_label_set_markup(GTK_LABEL(title), title_markup);
		g_free(title_markup);
	}
	gtk_label_set_xalign(GTK_LABEL(title), 0.0);
	gtk_box_pack_start(GTK_BOX(outer), title, FALSE, FALSE, 0);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(outer), scrolled, TRUE, TRUE, 0);

	grid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled), grid);

	drives = read_drive_list(&error);
	if (error)
	{
		GtkWidget *message = gtk_label_new(error->message);
		gtk_label_set_line_wrap(GTK_LABEL(message), TRUE);
		gtk_grid_attach(GTK_GRID(grid), message, 0, 0, 4, 1);
		g_error_free(error);
	}
	else if (drives->len == 0)
	{
		GtkWidget *message = gtk_label_new(_("No usable partitions found"));
		gtk_grid_attach(GTK_GRID(grid), message, 0, 0, 4, 1);
	}
	else
	{
		for (i = 0; i < drives->len; i++)
		{
			DriveInfo *drive = g_ptr_array_index(drives, i);
			DriveMenuAction *action = g_new0(DriveMenuAction, 1);
			GtkWidget *drive_button = drive_grid_button_new(drive);

			action->filer_window = filer_window;
			action->drive = drive_info_copy(drive);
			action->popover = popover;
			g_signal_connect(drive_button, "clicked",
				G_CALLBACK(drive_grid_activate), action);
			g_object_set_data_full(G_OBJECT(drive_button), "rox-drive-action",
				action, drive_menu_action_free);
			gtk_grid_attach(GTK_GRID(grid), drive_button,
				(gint) (i % 4), (gint) (i / 4), 1, 1);
		}
	}

	rows = drives->len > 0 ? (drives->len + 3) / 4 : 1;
	content_height = MIN(360, MAX(120, (gint) rows * 104 + 8));
	gtk_widget_set_size_request(scrolled, 584, content_height);
	g_ptr_array_free(drives, TRUE);

	gtk_widget_show_all(popover);
	gtk_popover_popup(GTK_POPOVER(popover));
}

/* Agregado por josejp2424: este botón no pertenece a la lista configurable,
 * por lo que siempre permanece visible mientras exista una barra de herramientas. */
GtkToolItem *drives_toolbar_button_new(FilerWindow *filer_window)
{
	GtkWidget *image;
	GtkToolItem *item;

	g_return_val_if_fail(filer_window != NULL, NULL);
	image = image_new_icon(DRIVE_ICON_INTERNAL, GTK_ICON_SIZE_LARGE_TOOLBAR);
	item = gtk_tool_button_new(image, _("Partitions"));
	/* Modificado por josejp2424 (2026): marcarlo como elemento importante
	 * y no homogéneo para que permanezca visible junto a Subir. */
	gtk_tool_item_set_is_important(item, TRUE);
	gtk_tool_item_set_homogeneous(item, FALSE);
	gtk_tool_item_set_tooltip_text(item,
		_("Show partitions, mount them and open their contents"));
	g_signal_connect(item, "clicked", G_CALLBACK(drives_button_clicked),
		filer_window);
	return item;
}

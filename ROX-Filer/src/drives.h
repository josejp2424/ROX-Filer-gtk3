/*
 * ROX-Filer GTK3 partition toolbar integration.
 *
 * Agregado por josejp2424 (2026): botón permanente de particiones,
 * detección de volúmenes, montaje y apertura desde la barra principal.
 *
 * Copyright (C) 2026 josejp2424
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 */

#ifndef _ROX_DRIVES_H
#define _ROX_DRIVES_H

#include <gtk/gtk.h>

/* Modificado por josejp2424 (2026): evitar incluir global.h desde este
 * encabezado. global.h se incluye primero desde cada unidad C y no dispone
 * de guardia de inclusión; incluirlo aquí redeclaraba sus enums al compilar
 * drives.c. La declaración adelantada mantiene este encabezado independiente. */
struct _FilerWindow;

GtkToolItem *drives_toolbar_button_new(struct _FilerWindow *filer_window);

#endif /* _ROX_DRIVES_H */

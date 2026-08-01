/*
 * Agregado por josejp2424 (2026): interfaz del modo ROX Desktop.
 */
#ifndef ROX_DESKTOP_H
#define ROX_DESKTOP_H

#include <glib.h>

void desktop_init(void);
void desktop_start(void);
gboolean desktop_is_running(void);

/* Herramientas independientes que pueden abrirse desde la línea de comandos. */
void desktop_open_wallpaper_manager(void);
void desktop_open_apps_manager(void);

/* Recalcula el área útil después de cambios externos de JWM/wbar. */
void desktop_refresh_after_environment_change(void);

/* Agregado por josejp2424 (2026): establece y guarda el wallpaper desde
 * acciones internas de ROX-Filer, sin scripts externos. */
gboolean desktop_set_wallpaper(const gchar *path, gboolean save,
                               GError **error);

#endif

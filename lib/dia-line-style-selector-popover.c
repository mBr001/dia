#include <glib.h>
#include <gtk/gtk.h>

#include "dia-line-style-selector.h"
#include "dia-line-style-selector-popover.h"
#include "dia_dirs.h"

typedef struct _DiaLineStyleSelectorPopoverPrivate DiaLineStyleSelectorPopoverPrivate;

struct _DiaLineStyleSelectorPopoverPrivate {
 GtkWidget *list;
 GtkWidget *length;
 GtkWidget *length_box;
};

G_DEFINE_TYPE_WITH_CODE (DiaLineStyleSelectorPopover, dia_line_style_selector_popover, GTK_TYPE_POPOVER,
                         G_ADD_PRIVATE (DiaLineStyleSelectorPopover))

enum {
  VALUE_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static gchar*
build_ui_filename (const gchar* name)
{
  gchar* uifile;

  if (g_getenv ("DIA_BASE_PATH") != NULL) {
    /* a small hack cause the final destination and the local path differ */
    const gchar* p = strrchr (name, '/');
    if (p != NULL)
      name = p+1;
    uifile = g_build_filename (g_getenv ("DIA_BASE_PATH"), "data", name, NULL);
  } else
    uifile = dia_get_data_directory (name);

  return uifile;
}

GtkWidget *
dia_line_style_selector_popover_new ()
{
  return g_object_new (DIA_TYPE_LINE_STYLE_SELECTOR_POPOVER, NULL);
}

LineStyle
dia_line_style_selector_popover_get_line_style (DiaLineStyleSelectorPopover *self,
                                                gdouble                     *length)
{
  DiaLineStyleSelectorPopoverPrivate *priv = dia_line_style_selector_popover_get_instance_private (self);
  GtkListBoxRow *row;
  GtkWidget *preview;
  LineStyle style;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (priv->list));
  preview = gtk_bin_get_child (GTK_BIN (row));
  style = dia_line_preview_get_line_style (DIA_LINE_PREVIEW (preview));

  /* Don't both getting this if we don't have somewhere to return it */
  if (length) {
    *length = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->length));
  }

  return style;
}

void
dia_line_style_selector_popover_set_line_style (DiaLineStyleSelectorPopover *self,
                                                LineStyle                    line_style)
{
  DiaLineStyleSelectorPopoverPrivate *priv = dia_line_style_selector_popover_get_instance_private (self);
  GtkListBoxRow *row;

  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (priv->list), line_style);
  gtk_list_box_select_row (GTK_LIST_BOX (priv->list), row);
}

void
dia_line_style_selector_popover_set_length (DiaLineStyleSelectorPopover *self,
                                            gdouble                      length)
{
  DiaLineStyleSelectorPopoverPrivate *priv = dia_line_style_selector_popover_get_instance_private (self);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->length), length);
}

static void
dia_line_style_selector_popover_class_init (DiaLineStyleSelectorPopoverClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  signals[VALUE_CHANGED] = g_signal_new ("value-changed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_FIRST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-line-style-selector-popover.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child_private (widget_class, DiaLineStyleSelectorPopover, list);
  gtk_widget_class_bind_template_child_private (widget_class, DiaLineStyleSelectorPopover, length);
  gtk_widget_class_bind_template_child_private (widget_class, DiaLineStyleSelectorPopover, length_box);

  g_object_unref (template_file);
}

static void
spin_change (GtkSpinButton *sb, gpointer data)
{
  g_signal_emit (G_OBJECT (data),
                 signals[VALUE_CHANGED], 0);
}

static void
row_selected (GtkListBox                  *box,
              GtkListBoxRow               *row,
              DiaLineStyleSelectorPopover *self)
{
  DiaLineStyleSelectorPopoverPrivate *priv = dia_line_style_selector_popover_get_instance_private (self);

  int state;
  state = dia_line_style_selector_popover_get_line_style (self, NULL)
     != LINESTYLE_SOLID;

  gtk_widget_set_sensitive (priv->length_box, state);
  g_signal_emit (G_OBJECT (self),
                 signals[VALUE_CHANGED], 0);
}

static void
dia_line_style_selector_popover_init (DiaLineStyleSelectorPopover *self)
{
  DiaLineStyleSelectorPopoverPrivate *priv = dia_line_style_selector_popover_get_instance_private (self);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_set (G_OBJECT (priv->length),
                "climb-rate", DEFAULT_LINESTYLE_DASHLEN,
                NULL);
  g_signal_connect (G_OBJECT (priv->length), "changed",
                    G_CALLBACK (spin_change), self);

  for (int i = 0; i <= LINESTYLE_DOTTED; i++) {
    GtkWidget *style = dia_line_preview_new (i);
    gtk_widget_show (style);
    gtk_list_box_insert (GTK_LIST_BOX (priv->list), style, i);
  }

  g_signal_connect (G_OBJECT (priv->list), "row-selected",
                    G_CALLBACK (row_selected), self);
}
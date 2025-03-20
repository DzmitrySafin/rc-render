#include "rc-render-main.h"

static RenderParams * render_params;
static GtkWidget * main_window;
static GtkWidget * file_button_json;
static GtkWidget * file_label_json;
static GtkWidget * file_button_bg;
static GtkWidget * file_label_bg;
static GtkWidget * font_button_label;
static GtkWidget * font_button_algorithm;
static GtkWidget * label_size;

static void set_size_labels ()
{
  CubeParams data;
  gchar buffer[BUFFER_SIZE_MIDDLE];

  calculate_params (&data);

  g_snprintf (buffer, BUFFER_SIZE_MIDDLE, "Image size: %d x %d px", data.image_width, data.image_height);
  gtk_label_set_label (GTK_LABEL (label_size), buffer);
}

/* CALLBACKS */

static void
adjustment_value_callback (GtkAdjustment* adjustment, gpointer data)
{
  gimp_int_adjustment_update (adjustment, data);
  set_size_labels ();
}

static void
check_toggle_callback (GtkWidget* widget, gpointer data)
{
  gimp_toggle_button_update (widget, data);
}

static void
check_toggle_file_callback (GtkWidget* widget, RenderFile* data)
{
  gimp_toggle_button_update (widget, &data->use);
  gtk_widget_set_sensitive (file_button_json, data->use);
  if (data->use && data->fullname[0] == '\0') {
    gtk_widget_activate (file_button_json);
  }
}

static void
check_toggle_font_callback (GtkWidget* widget, gpointer data)
{
  gimp_toggle_button_update (widget, data);
  gtk_widget_set_sensitive (font_button_label, render_params->font_label.use);
  gtk_widget_set_sensitive (font_button_algorithm, render_params->font_algorithm.use);
  set_size_labels ();
}

static void
radio_toggle_callback (GtkWidget* widget, gpointer data)
{
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
    return;
  }

  BackgroundType bgt = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "gimp-item-data"));

  render_params->bg_type = bgt;
  render_params->bg_file.use = bgt == BG_CUSTOM;
  gtk_widget_set_sensitive (file_button_bg, render_params->bg_file.use);
  if (render_params->bg_file.use && render_params->bg_file.fullname[0] == '\0') {
    gtk_widget_activate (file_button_bg);
  }
}

static void
choose_file_json_callback (GtkWidget* widget, RenderFile* data)
{
  GtkWidget     *dialog;
  GtkFileFilter *filter;
  gint   response_id;
  gchar *fullname;
  gchar *filename;

  dialog = gtk_file_chooser_dialog_new (
    _("Open JSON file for Rubik's Cube configurations"),
    GTK_WINDOW (main_window),
    GTK_FILE_CHOOSER_ACTION_OPEN,
    _("_Cancel"), GTK_RESPONSE_CANCEL,
    _("_Open"), GTK_RESPONSE_OK,
    NULL);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("JSON files"));
  gtk_file_filter_add_mime_type (filter, N_("application/json"));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  response_id = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response_id == GTK_RESPONSE_OK) {
    fullname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if (fullname != NULL) {
      g_strlcpy (data->fullname, fullname, PATH_MAX);
      filename = g_path_get_basename (fullname);
      gtk_label_set_text (GTK_LABEL (file_label_json), filename);
      g_free (filename);
      g_free (fullname);
    }
  }

  gtk_widget_destroy (dialog);
}

static void
choose_file_bg_callback (GtkWidget* widget, RenderFile* data)
{
  GtkWidget     *dialog;
  GtkFileFilter *filter;
  gint   response_id;
  gchar *fullname;
  gchar *filename;

  dialog = gtk_file_chooser_dialog_new (
    _("Open image file to be used as background"),
    GTK_WINDOW (main_window),
    GTK_FILE_CHOOSER_ACTION_OPEN,
    _("_Cancel"), GTK_RESPONSE_CANCEL,
    _("_Open"), GTK_RESPONSE_OK,
    NULL);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Image files"));
  gtk_file_filter_add_pixbuf_formats (filter);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  response_id = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response_id == GTK_RESPONSE_OK) {
    fullname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if (fullname != NULL) {
      g_strlcpy (data->fullname, fullname, PATH_MAX);
      filename = g_path_get_basename (fullname);
      gtk_label_set_text (GTK_LABEL (file_label_bg), filename);
      g_free (filename);
      g_free (fullname);
    }
  }

  gtk_widget_destroy (dialog);
}

static void
font_set_callback (GtkWidget* widget, RenderFont* data)
{
  const gchar *font_name;
  PangoFontDescription *font;

  font_name = gtk_font_button_get_font_name (GTK_FONT_BUTTON (widget));
  font = pango_font_description_from_string (font_name);

  g_strlcpy (data->fullname, font_name, BUFFER_SIZE_LONG);
  data->size = PANGO_PIXELS (pango_font_description_get_size (font));

  pango_font_description_free (font);
  set_size_labels ();
}

/* NOTEBOOK PAGES */

static GtkWidget *
create_spin_button (
  GtkAdjustment** adjustment,
  gdouble         value,
  gdouble         lower,
  gdouble         upper,
  gdouble         step_increment,
  gdouble         page_increment,
  gdouble         page_size,
  gdouble         climb_rate,
  guint           digits)
{
  GtkWidget *spinbutton;

  *adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (value, lower, upper, step_increment, page_increment, page_size));

  spinbutton = gimp_spin_button_new (*adjustment, climb_rate, digits);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  return spinbutton;
}

static GtkWidget *
create_algorithm_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *widget;
  GtkWidget *frameF, *hboxF;
  gchar     *filename;
  gchar      buffer[BUFFER_SIZE_MIDDLE];

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  /* Frame #1 - predefined combinations */
  frame = gimp_frame_new (_("Predefined combinations"));
  gtk_container_add (GTK_CONTAINER (page), frame);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* F2L toggle */
  g_snprintf (buffer, BUFFER_SIZE_MIDDLE, "F2L - %u algorithms", count_f2l ());
  widget = gtk_check_button_new_with_label (buffer);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->render_f2l);
  gimp_help_set_help_data (widget, HELP_RENDER_F2L, NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (check_toggle_callback), &render_params->render_f2l);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* OLL toggle */
  g_snprintf (buffer, BUFFER_SIZE_MIDDLE, "OLL - %u algorithms", count_oll ());
  widget = gtk_check_button_new_with_label (buffer);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->render_oll);
  gimp_help_set_help_data (widget, HELP_RENDER_OLL, NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (check_toggle_callback), &render_params->render_oll);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* PLL toggle */
  g_snprintf (buffer, BUFFER_SIZE_MIDDLE, "PLL - %u algorithms", count_pll ());
  widget = gtk_check_button_new_with_label (buffer);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->render_pll);
  gimp_help_set_help_data (widget, HELP_RENDER_PLL, NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (check_toggle_callback), &render_params->render_pll);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* Frame #2 - custom combinations */
  frame = gimp_frame_new (_("Custom combinations"));
  gtk_container_add (GTK_CONTAINER (page), frame);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* horizontal panel for file selection */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* file toggle */
  widget = gtk_check_button_new_with_label (N_("File:"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->json_file.use);
  gimp_help_set_help_data (widget, HELP_RENDER_FILE, NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (check_toggle_file_callback), &render_params->json_file);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* file name - label inside horizontal frame */
  frameF = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frameF, TRUE, TRUE, 0);
  gtk_widget_show (frameF);

  hboxF = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (frameF), hboxF);
  gtk_widget_show (hboxF);

  if (render_params->bg_file.fullname[0] == '\0') {
    file_label_json = gtk_label_new (NULL);
  } else {
    filename = g_path_get_basename (render_params->json_file.fullname);
    file_label_json = gtk_label_new (filename);
    g_free (filename);
  }
  gimp_help_set_help_data (file_label_json, HELP_FILE_ALGORITHMS, NULL);
  gtk_box_pack_start (GTK_BOX (hboxF), file_label_json, FALSE, FALSE, 6);
  gtk_widget_show (file_label_json);

  /* open file button */
  file_button_json = gtk_button_new ();
  gtk_widget_set_sensitive (file_button_json, render_params->json_file.use);
  gtk_button_set_focus_on_click (GTK_BUTTON (file_button_json), FALSE);
  gtk_button_set_relief (GTK_BUTTON (file_button_json), GTK_RELIEF_NONE);
  gtk_button_set_image (GTK_BUTTON (file_button_json), gtk_image_new_from_icon_name (GIMP_ICON_DOCUMENT_OPEN, GTK_ICON_SIZE_BUTTON));
  g_signal_connect (file_button_json, "clicked", G_CALLBACK (choose_file_json_callback), &render_params->json_file);
  gtk_box_pack_start (GTK_BOX (hbox), file_button_json, FALSE, FALSE, 0);
  gtk_widget_show (file_button_json);

  gtk_widget_show (page);
  return page;
}

static GtkWidget *
create_size_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *widget;
  GtkAdjustment *adjCell, *adjLine, *adjOll;

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  /* Frame #1 - size parameters */
  frame = gimp_frame_new (_("Image size parameters"));
  gtk_container_add (GTK_CONTAINER (page), frame);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 6);
  gtk_widget_show (hbox);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 6);
  gtk_widget_show (table);

  /* cell size spin */
  widget = create_spin_button (&adjCell, render_params->cell_size,
                                10, 10000,
                                1.0, 10.0,
                                0.0, 0.0, 0);
  g_signal_connect (adjCell, "value-changed", G_CALLBACK (adjustment_value_callback), &render_params->cell_size);
  gimp_help_set_help_data (widget, HELP_CELL_SIZE, NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Cell size:"), 0.0, 0.5,
                             widget, 1, FALSE);

  /* line width spin */
  widget = create_spin_button (&adjLine, render_params->line_width,
                                1, 100,
                                1.0, 10.0,
                                0.0, 0.0, 0);
  g_signal_connect (adjLine, "value-changed", G_CALLBACK (adjustment_value_callback), &render_params->line_width);
  gimp_help_set_help_data (widget, HELP_LINE_WIDTH, NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Line width:"), 0.0, 0.5,
                             widget, 1, FALSE);

  /* OLL side width spin */
  widget = create_spin_button (&adjOll, render_params->oll_side_width,
                                1, 100,
                                1.0, 10.0,
                                0.0, 0.0, 0);
  g_signal_connect (adjOll, "value-changed", G_CALLBACK (adjustment_value_callback), &render_params->oll_side_width);
  gimp_help_set_help_data (widget, HELP_OLL_WIDTH, NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("OLL side width:"), 0.0, 0.5,
                             widget, 1, FALSE);

  /* Frame #2 - text/font parameters */
  frame = gimp_frame_new (_("Text / Fonts"));
  gtk_container_add (GTK_CONTAINER (page), frame);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 6);
  gtk_widget_show (hbox);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 6);
  gtk_widget_show (table);

  /* Label toggle */
  widget = gtk_check_button_new_with_label (N_("Render label"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->font_label.use);
  gimp_help_set_help_data (widget, HELP_RENDER_LABEL, NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (check_toggle_font_callback), &render_params->font_label.use);
  gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (widget);

  /* Label font */
  font_button_label = gtk_font_button_new_with_font (render_params->font_label.fullname);
  gtk_font_button_set_title (GTK_FONT_BUTTON (font_button_label), "Pick font for label");
  gtk_widget_set_sensitive (font_button_label, render_params->font_label.use);
  gimp_help_set_help_data (font_button_label, _("Label (upper right corner) font"), NULL);
  g_signal_connect (font_button_label, "font-set", G_CALLBACK (font_set_callback), &render_params->font_label);
  gtk_table_attach (GTK_TABLE (table), font_button_label, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (font_button_label);

  /* Algorithm toggle */
  widget = gtk_check_button_new_with_label (N_("Render algorithm"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->font_algorithm.use);
  gimp_help_set_help_data (widget, HELP_RENDER_ALGORITHM, NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (check_toggle_font_callback), &render_params->font_algorithm.use);
  gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (widget);

  /* Algorithm font */
  font_button_algorithm = gtk_font_button_new_with_font (render_params->font_algorithm.fullname);
  gtk_font_button_set_title (GTK_FONT_BUTTON (font_button_algorithm), "Pick font for algorithm");
  gtk_widget_set_sensitive (font_button_algorithm, render_params->font_algorithm.use);
  gimp_help_set_help_data (font_button_algorithm, _("Algorithm (image bottom) font"), NULL);
  g_signal_connect (font_button_algorithm, "font-set", G_CALLBACK (font_set_callback), &render_params->font_algorithm);
  gtk_table_attach (GTK_TABLE (table), font_button_algorithm, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (font_button_algorithm);

  /* out of frames */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (page), hbox);
  gtk_widget_show (hbox);

  label_size = gtk_label_new (N_(""));
  gtk_box_pack_start (GTK_BOX (hbox), label_size, FALSE, FALSE, 6);
  gtk_widget_show (label_size);

  set_size_labels ();
  gtk_widget_show (page);
  return page;
}

static GtkWidget *
create_color_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *colorbutton;

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  /* Frame #1 - cube colors */
  frame = gimp_frame_new (_("Cube faces"));
  gtk_container_add (GTK_CONTAINER (page), frame);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* top side color */
  colorbutton = gimp_color_button_new (_("Select top side color"),
                                       48, 12,
                                       &render_params->color_top,
                                       GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Top side:"), 0.0, 0.5,
                             colorbutton, 1, TRUE);
  g_signal_connect (colorbutton, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &render_params->color_top);
  gimp_help_set_help_data (colorbutton, HELP_TOP_COLOR, NULL);

  /* front side color */
  colorbutton = gimp_color_button_new (_("Select front side color"),
                                       48, 12,
                                       &render_params->color_front,
                                       GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Front side:"), 0.0, 0.5,
                             colorbutton, 1, TRUE);
  g_signal_connect (colorbutton, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &render_params->color_front);
  gimp_help_set_help_data (colorbutton, HELP_FRONT_COLOR, NULL);

  /* down side color */
  colorbutton = gimp_color_button_new (_("Select down side color"),
                                       48, 12,
                                       &render_params->color_down,
                                       GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 0,
                             _("Down side:"), 0.0, 0.5,
                             colorbutton, 1, TRUE);
  g_signal_connect (colorbutton, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &render_params->color_down);
  gimp_help_set_help_data (colorbutton, HELP_DOWN_COLOR, NULL);

  /* right side color */
  colorbutton = gimp_color_button_new (_("Select right side color"),
                                       48, 12,
                                       &render_params->color_right,
                                       GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 2, 1,
                             _("Right side:"), 0.0, 0.5,
                             colorbutton, 1, TRUE);
  g_signal_connect (colorbutton, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &render_params->color_right);
  gimp_help_set_help_data (colorbutton, HELP_RIGHT_COLOR, NULL);

  /* neutral color */
  colorbutton = gimp_color_button_new (_("Select neutral color"),
                                       48, 12,
                                       &render_params->color_neutral,
                                       GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Neutral side:"), 0.0, 0.5,
                             colorbutton, 1, TRUE);
  g_signal_connect (colorbutton, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &render_params->color_neutral);
  gimp_help_set_help_data (colorbutton, HELP_NEUTRAL_COLOR, NULL);

  /* Frame #2 - lines colors */
  frame = gimp_frame_new (_("Draw PLL arrows"));
  gtk_container_add (GTK_CONTAINER (page), frame);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* primary arrow color */
  colorbutton = gimp_color_button_new (_("Select primary arrow color"),
                                       48, 12,
                                       &render_params->color_line1,
                                       GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Primary:"), 0.0, 0.5,
                             colorbutton, 1, TRUE);
  g_signal_connect (colorbutton, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &render_params->color_line1);
  gimp_help_set_help_data (colorbutton, HELP_LINE1_COLOR, NULL);

  /* secondary arrow color */
  colorbutton = gimp_color_button_new (_("Select secondary arrow color"),
                                       48, 12,
                                       &render_params->color_line2,
                                       GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Secondary:  "), 0.0, 0.5,
                             colorbutton, 1, TRUE);
  g_signal_connect (colorbutton, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &render_params->color_line2);
  gimp_help_set_help_data (colorbutton, HELP_LINE2_COLOR, NULL);

  gtk_widget_show (page);
  return page;
}

static GtkWidget *
create_miscellaneous_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *widget;
  GtkWidget *table;
  GtkWidget *frameF, *hboxF;
  GSList    *group = NULL;
  gchar     *filename;

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = gimp_frame_new (_("Background"));
  gtk_container_add (GTK_CONTAINER (page), frame);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  widget = gtk_radio_button_new_with_mnemonic (group, N_("_Default"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));
  g_object_set_data (G_OBJECT (widget), "gimp-item-data", GINT_TO_POINTER (BG_DEFAULT));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->bg_type == BG_DEFAULT);
  gimp_help_set_help_data (widget, _("Predefined background image"), NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (radio_toggle_callback), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gtk_radio_button_new_with_mnemonic (group, N_("_Transparent"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));
  g_object_set_data (G_OBJECT (widget), "gimp-item-data", GINT_TO_POINTER (BG_TRANSPARENT));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->bg_type == BG_TRANSPARENT);
  gimp_help_set_help_data (widget, _("Transparent background"), NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (radio_toggle_callback), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* table to align color and image selection widgets */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 6);
  gtk_widget_show (table);

  /* color selection */
  widget = gtk_radio_button_new_with_mnemonic (group, N_("_Color"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));
  g_object_set_data (G_OBJECT (widget), "gimp-item-data", GINT_TO_POINTER (BG_COLOR));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->bg_type == BG_COLOR);
  gimp_help_set_help_data (widget, _("Custom background color"), NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (radio_toggle_callback), NULL);
  gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (widget);

  widget = gimp_color_button_new (_("Select background color"),
                                  48, 12,
                                  &render_params->bg_color,
                                  GIMP_COLOR_AREA_FLAT);
  gimp_help_set_help_data (widget, _("Image background color"), NULL);
  g_signal_connect (widget, "color-changed", G_CALLBACK (gimp_color_button_get_color), &render_params->bg_color);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (widget);

  /* image selection */
  widget = gtk_radio_button_new_with_mnemonic (group, N_("Select _image"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));
  g_object_set_data (G_OBJECT (widget), "gimp-item-data", GINT_TO_POINTER (BG_CUSTOM));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), render_params->bg_type == BG_CUSTOM);
  gimp_help_set_help_data (widget, _("Custom background image"), NULL);
  g_signal_connect (widget, "toggled", G_CALLBACK (radio_toggle_callback), NULL);
  gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (widget);

  /* file name - label inside horizontal frame */
  frameF = gtk_frame_new (NULL);
  gtk_table_attach (GTK_TABLE (table), frameF, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (frameF);

  hboxF = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (frameF), hboxF);
  gtk_widget_show (hboxF);

  if (render_params->bg_file.fullname[0] == '\0') {
    file_label_bg = gtk_label_new (NULL);
  } else {
    filename = g_path_get_basename (render_params->bg_file.fullname);
    file_label_bg = gtk_label_new (filename);
    g_free (filename);
  }
  gtk_box_pack_start (GTK_BOX (hboxF), file_label_bg, FALSE, FALSE, 6);
  gtk_widget_show (file_label_bg);

  /* open file button */
  file_button_bg = gtk_button_new ();
  gtk_widget_set_sensitive (file_button_bg, render_params->bg_file.use);
  gtk_button_set_focus_on_click (GTK_BUTTON (file_button_bg), FALSE);
  gtk_button_set_relief (GTK_BUTTON (file_button_bg), GTK_RELIEF_NONE);
  gtk_button_set_image (GTK_BUTTON (file_button_bg), gtk_image_new_from_icon_name (GIMP_ICON_DOCUMENT_OPEN, GTK_ICON_SIZE_BUTTON));
  g_signal_connect (file_button_bg, "clicked", G_CALLBACK (choose_file_bg_callback), &render_params->bg_file);
  gtk_table_attach (GTK_TABLE (table), file_button_bg, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (file_button_bg);

  gtk_widget_show (page);
  return page;
}

/* MAIN */

gboolean
main_dialog(RenderParams* params)
{
  gboolean ok;
  GtkWidget *dialog;
  GtkWidget *notebook;
  GtkWidget *page;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *button;

  g_assert (params != NULL);

  render_params = params;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);
  dialog = gimp_dialog_new (_("Rubik's Cube settings"), PLUG_IN_ROLE,
           NULL, 0,
           gimp_standard_help_func, PLUG_IN_PROC,
           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
           GTK_STOCK_OK, GTK_RESPONSE_OK,
           NULL);
  main_window = dialog;

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
  gimp_window_set_transient (GTK_WINDOW (dialog));

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), notebook, TRUE, TRUE, 0);

  page = create_algorithm_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, gtk_label_new_with_mnemonic (_("_Algorithms")));

  page = create_size_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, gtk_label_new_with_mnemonic (_("_Size")));

  page = create_color_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, gtk_label_new_with_mnemonic (_("_Colors")));

  page = create_miscellaneous_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, gtk_label_new_with_mnemonic (_("_Miscellaneous")));

  gtk_widget_show (notebook);
  gtk_widget_show (dialog);

  ok = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);
  return ok;
}

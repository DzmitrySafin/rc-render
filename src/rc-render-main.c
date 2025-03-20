#include "rc-render-main.h"
#define _USE_MATH_DEFINES
#include <math.h>

/* GLOBAL */

static RenderParams render_params = {
  300, 10, 30, 40,
  { 1.0, 1.0, 0.0, 1.0 },
  { 1.0, 0.0, 0.0, 1.0 },
  { 1.0, 1.0, 1.0, 1.0 },
  { 0.0, 1.0, 0.0, 1.0 },
  { 0.37, 0.37, 0.37, 1.0 },
  { 0.0, 0.0, 1.0, 1.0 },
  { 1.0, 0.0, 0.0, 1.0 },
  { 0.0, 0.0, 0.0, 1.0 },
  TRUE, TRUE, TRUE,
  { FALSE, "\0" },
  { TRUE, 32, "Chalkboard Bold 32" },
  { TRUE, 56, "Chalkboard Bold 56" },
  BG_DEFAULT,
  { 0.0, 1.0, 0.5, 1.0 },
  { FALSE, "\0" }
};
static CubeParams cube_params = { 0 };
static RenderData render_data = { 0 };

static GError * json_error = NULL;
static GArray * cube_data = NULL;


/* DRAW */

static void
draw_f2l_colors (GeglBuffer* buffer, guchar data[])
{
  gint32 step, i, j;
  GeglRectangle rect = { 0, 0, cube_params.layer_size, cube_params.layer_size };

  gegl_buffer_set_color (buffer, &rect, render_data.color_black);
  step = render_params.line_width + render_params.cell_size;

  for (j = 0; j < CUBE_NUMBER; j++) {
    for (i = 0; i < CUBE_NUMBER; i++) {
      gegl_rectangle_set (&rect, render_params.line_width + step * i, render_params.line_width + step * j, render_params.cell_size, render_params.cell_size);
      switch (data[j * CUBE_NUMBER + i]) {
        case CUBE_COLOR_TOP:
          gegl_buffer_set_color (buffer, &rect, render_data.color_top);
          break;
        case CUBE_COLOR_FRONT:
          gegl_buffer_set_color (buffer, &rect, render_data.color_front);
          break;
        case CUBE_COLOR_RIGHT:
          gegl_buffer_set_color (buffer, &rect, render_data.color_right);
          break;
        case CUBE_COLOR_DOWN:
          gegl_buffer_set_color (buffer, &rect, render_data.color_down);
          break;
        case CUBE_COLOR_NEUTRAL:
          gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);
          break;
      }
    }
  }
}

static void
render_f2l_cube (guchar data[])
{
  gint noutput;

  draw_f2l_colors (render_data.buffer_top, &data[0]);
  draw_f2l_colors (render_data.buffer_front, &data[CUBE_NUMBER * CUBE_NUMBER]);
  draw_f2l_colors (render_data.buffer_right, &data[CUBE_NUMBER * CUBE_NUMBER * 2]);

  gegl_buffer_flush (render_data.buffer_top);
  gegl_buffer_flush (render_data.buffer_front);
  gegl_buffer_flush (render_data.buffer_right);

  GimpParam input[] = {
    { GIMP_PDB_INT32, { .d_int32 = GIMP_RUN_NONINTERACTIVE } },
    { GIMP_PDB_IMAGE, { .d_image = render_data.image_id } },
    { GIMP_PDB_DRAWABLE, { .d_drawable = render_data.layer_draw } },
    { GIMP_PDB_INT32, { .d_int32 = 2     } }, // type of mapping - box
    { GIMP_PDB_FLOAT, { .d_float = 0.50  } }, // position of viewpoint
    { GIMP_PDB_FLOAT, { .d_float = 0.50  } },
    { GIMP_PDB_FLOAT, { .d_float = 2.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.50  } }, // object position
    { GIMP_PDB_FLOAT, { .d_float = 0.46  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 1.00  } }, // first axis of object
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } }, // second axis of object
    { GIMP_PDB_FLOAT, { .d_float = 1.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 35.00 } }, // rotation about X/Y/Z axis in degrees
    { GIMP_PDB_FLOAT, { .d_float = 45.00 } },
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } },
    { GIMP_PDB_INT32, { .d_int32 = 2     } }, // type of lightsource - none
    { GIMP_PDB_COLOR, { .d_color = { 0.0, 0.0, 0.0, 1.0 } } }, // lightsource color
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } }, // lightsource position
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } }, // lightsource direction
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.30  } }, // material ambient intensity (0..1)
    { GIMP_PDB_FLOAT, { .d_float = 1.00  } }, // material diffuse intensity (0..1)
    { GIMP_PDB_FLOAT, { .d_float = 0.50  } }, // material diffuse reflectivity (0..1)
    { GIMP_PDB_FLOAT, { .d_float = 0.50  } }, // material specular reflectivity (0..1)
    { GIMP_PDB_FLOAT, { .d_float = 27.00 } }, // material highlight (0..->)
    { GIMP_PDB_INT32, { .d_int32 = TRUE  } }, // apply antialiasing
    { GIMP_PDB_INT32, { .d_int32 = FALSE } }, // tile source image
    { GIMP_PDB_INT32, { .d_int32 = FALSE } }, // create a new image
    { GIMP_PDB_INT32, { .d_int32 = TRUE  } }, // make background transparent
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } }, // sphere/cylinder radius
    { GIMP_PDB_FLOAT, { .d_float = 0.60  } }, // box X/Y/Z size (0..->)
    { GIMP_PDB_FLOAT, { .d_float = 0.60  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.60  } },
    { GIMP_PDB_FLOAT, { .d_float = 0.00  } }, // cylinder length (0..->)
    { GIMP_PDB_DRAWABLE, { .d_drawable = render_data.layer_front } }, // box front face
    { GIMP_PDB_DRAWABLE, { .d_drawable = -1 } }, // box back face
    { GIMP_PDB_DRAWABLE, { .d_drawable = render_data.layer_top   } }, // box top face
    { GIMP_PDB_DRAWABLE, { .d_drawable = -1 } }, // box bottom face
    { GIMP_PDB_DRAWABLE, { .d_drawable = render_data.layer_right } }, // box left face
    { GIMP_PDB_DRAWABLE, { .d_drawable = -1 } }, // box right face
    { GIMP_PDB_DRAWABLE, { .d_drawable = -1 } }, // cylinder top face
    { GIMP_PDB_DRAWABLE, { .d_drawable = -1 } }, // cylinder bottom face
  };

  GimpParam *output = gimp_run_procedure2 ("plug-in-map-object", &noutput, G_N_ELEMENTS (input), input);
  gimp_destroy_params (output, noutput);
}

static void
draw_oll_colors (GeglBuffer* buffer, guchar data[])
{
  gint32 step, offset, far_offset, i, j;
  GeglRectangle rect = { 0, 0, cube_params.layer_size, cube_params.layer_size };

  gegl_buffer_set_color (buffer, &rect, render_data.color_black);
  step = render_params.line_width + cube_params.oll_cell_size;
  offset = cube_params.oll_border_width + cube_params.oll_side_width + render_params.line_width;
  far_offset = cube_params.layer_size - cube_params.oll_border_width - cube_params.oll_side_width;

  gegl_rectangle_set (&rect, cube_params.oll_border_width, cube_params.oll_border_width, cube_params.oll_side_width, cube_params.oll_side_width);
  gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);
  gegl_rectangle_set (&rect, far_offset, cube_params.oll_border_width, cube_params.oll_side_width, cube_params.oll_side_width);
  gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);
  gegl_rectangle_set (&rect, far_offset, far_offset, cube_params.oll_side_width, cube_params.oll_side_width);
  gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);
  gegl_rectangle_set (&rect, cube_params.oll_border_width, far_offset, cube_params.oll_side_width, cube_params.oll_side_width);
  gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);

  for (i = 0; i < CUBE_NUMBER; i++) {
    gegl_rectangle_set (&rect, offset + step * i, cube_params.oll_border_width, cube_params.oll_cell_size, cube_params.oll_side_width);
    switch (data[1 + i]) {
      case CUBE_COLOR_TOP:
        gegl_buffer_set_color (buffer, &rect, render_data.color_top);
        break;
      case CUBE_COLOR_NEUTRAL:
        gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);
        break;
    }
    gegl_rectangle_set (&rect, offset + step * i, cube_params.layer_size - cube_params.oll_border_width - cube_params.oll_side_width, cube_params.oll_cell_size, cube_params.oll_side_width);
    switch (data[CUBE_NUMBER_EX * (CUBE_NUMBER_EX - 1) + 1 + i]) {
      case CUBE_COLOR_TOP:
        gegl_buffer_set_color (buffer, &rect, render_data.color_top);
        break;
      case CUBE_COLOR_NEUTRAL:
        gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);
        break;
    }
    gegl_rectangle_set (&rect, cube_params.oll_border_width, offset + step * i, cube_params.oll_side_width, cube_params.oll_cell_size);
    switch (data[CUBE_NUMBER_EX * (i + 1)]) {
      case CUBE_COLOR_TOP:
        gegl_buffer_set_color (buffer, &rect, render_data.color_top);
        break;
      case CUBE_COLOR_NEUTRAL:
        gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);
        break;
    }
    gegl_rectangle_set (&rect, cube_params.layer_size - cube_params.oll_border_width - cube_params.oll_side_width, offset + step * i, cube_params.oll_side_width, cube_params.oll_cell_size);
    switch (data[CUBE_NUMBER_EX * (i + 2) - 1]) {
      case CUBE_COLOR_TOP:
        gegl_buffer_set_color (buffer, &rect, render_data.color_top);
        break;
      case CUBE_COLOR_NEUTRAL:
        gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);
        break;
    }
  }

  for (j = 0; j < CUBE_NUMBER; j++) {
    for (i = 0; i < CUBE_NUMBER; i++) {
      gegl_rectangle_set (&rect, offset + step * i, offset + step * j, cube_params.oll_cell_size, cube_params.oll_cell_size);
      switch (data[(j + 1) * CUBE_NUMBER_EX + i + 1]) {
        case CUBE_COLOR_TOP:
          gegl_buffer_set_color (buffer, &rect, render_data.color_top);
          break;
        case CUBE_COLOR_NEUTRAL:
          gegl_buffer_set_color (buffer, &rect, render_data.color_neutral);
          break;
      }
    }
  }
}

static GeglNode *
draw_pll_arrow (GeglNode* node, GeglColor* color, gint32 row1, gint32 col1, gint32 row2, gint32 col2)
{
  gdouble angle = 0;
  if (col1 == col2) {
    angle = row2 < row1 ? 0 : 180;
  } else if (row1 == row2) {
    angle = col2 < col1 ? 90 : -90;
  } else {
    angle = atan ((gdouble)(row2 - row1) / (col2 - col1)) * 180 / M_PI;
    if (row2 > row1) {
      angle += 180;
    }
  }

  gdouble line_width = render_params.line_width;
  gdouble half_width = line_width / 2;
  gdouble length = sqrt ((col2 - col1) * (col2 - col1) + (row2 - row1) * (row2 - row1)) * (render_params.cell_size + render_params.line_width);
  gdouble top_length = length / 2;
  gdouble bottom_length = length / 2 - line_width;
  gdouble head_height = line_width * 8.0;
  gdouble wing_angle = 20.0 * M_PI / 180;
  gdouble wing_width = head_height * sin (wing_angle);
  gdouble tip_height = half_width / tan (wing_angle);
  gdouble tip_width = tip_height * sin (wing_angle);
  gdouble corner_radius = 4.0;
  gdouble corner_bezier = corner_radius * 0.55;
  gdouble tip_bezier = tip_width * 0.4;
  gdouble inner_height = (wing_width - line_width - half_width) / tan (wing_angle);
  // guint x1 = render_params.cell_size * (col1 + 0.5) + render_params.line_width * (col1 + 1);
  // guint y1 = render_params.cell_size * (row1 + 0.5) + render_params.line_width * (row1 + 1);
  // guint x2 = render_params.cell_size * (col2 + 0.5) + render_params.line_width * (col2 + 1);
  // guint y2 = render_params.cell_size * (row2 + 0.5) + render_params.line_width * (row2 + 1);
  guint xM = (render_params.line_width + (render_params.cell_size + render_params.line_width) * (col1 + col2 + 1)) / 2;
  guint yM = (render_params.line_width + (render_params.cell_size + render_params.line_width) * (row1 + row2 + 1)) / 2;

  gdouble min_x, max_x, min_y, max_y;
  gegl_path_get_bounds (render_data.path_arrow, &min_x, &max_x, &min_y, &max_y);
  gdouble scale = length / (max_y - min_y);

  // g_print ("FIRST point: %d x %d; SECOND point: %d x %d\n", col1, row1, col2, row2);
  // g_print ("MIDDLE point: %d x %d\n", xM, yM);
  // g_print ("ANGLE: %f\n", angle);
  // g_print ("WIDTH: %f; LENGTH: %f\n", line_width, length);
  // g_print ("HEAD height: %f; TIP height: %f; TIP width: %f\n", head_height, tip_height, tip_width);
  // g_print ("WING width: %f; CORNER Bezier: %f\n", wing_width, corner_bezier);

  GeglPath *path = gegl_path_new ();
  gegl_path_append (path, 'M', 0.0, 0.0);
  gegl_path_append (path, 'm', 0.0, -top_length + tip_height - tip_width);
  /* tip left radius + left wing */
  gegl_path_append (path, 'C', -tip_bezier, -top_length + tip_height - tip_width,
                               -tip_width, -top_length + tip_height - tip_bezier,
                               -tip_width, -top_length + tip_height);
  gegl_path_append (path, 'L', -wing_width, -top_length + head_height);
  //
  gegl_path_append (path, 'L', -wing_width + line_width, -top_length + head_height);
  gegl_path_append (path, 'L', -half_width, -top_length + head_height - inner_height);
  /* line with rounded bottom corners */
  gegl_path_append (path, 'L', -half_width, bottom_length - corner_radius);
  gegl_path_append (path, 'C', -half_width, bottom_length - corner_radius + corner_bezier,
                               -corner_bezier, bottom_length,
                               -half_width + corner_radius, bottom_length);
  gegl_path_append (path, 'l', line_width - corner_radius - corner_radius, 0.0);
  gegl_path_append (path, 'C', corner_bezier, bottom_length,
                               half_width, bottom_length - corner_radius + corner_bezier,
                               half_width, bottom_length - corner_radius);
  gegl_path_append (path, 'L', half_width, -top_length + head_height - inner_height);
  //
  gegl_path_append (path, 'L', wing_width - line_width, -top_length + head_height);
  gegl_path_append (path, 'L', wing_width, -top_length + head_height);
  /* right wing + tip right radius */
  gegl_path_append (path, 'L', tip_width, -top_length + tip_height);
  gegl_path_append (path, 'C', tip_width, -top_length + tip_height - tip_bezier,
                               tip_bezier, -top_length + tip_height - tip_width,
                               0.0, -top_length + tip_height - tip_width);
  gegl_path_append (path, 'z');

  // setlocale (LC_NUMERIC, "en_US.UTF-8");
  // g_print ("GEGL PATH: %s\n", gegl_path_to_string (path));
  // setlocale (LC_NUMERIC, NULL);

  GeglNode *node_path = gegl_node_new_child (node, "operation", "gegl:fill-path",
                      "color", color,
                      "d", path, NULL);
  GeglNode *node_path1 = gegl_node_new_child (node, "operation", "gegl:fill-path",
                      "color", color,
                      "d", render_data.path_arrow, NULL);
  g_object_unref (path);
  GeglNode *node_scale = gegl_node_new_child (node, "operation", "gegl:scale-size",
                      "x", max_x - min_x,
                      "y", length - line_width, NULL);
  GeglNode *node_rotate = gegl_node_new_child (node, "operation", "gegl:rotate",
                      "degrees", angle, NULL);
  GeglNode *node_translate = gegl_node_new_child (node, "operation", "gegl:translate",
                      "x", (gdouble)xM,
                      "y", (gdouble)yM, NULL);
  GeglNode *node_over = gegl_node_new_child (node, "operation", "gegl:over", NULL);

  gegl_node_link_many (node_path, node_rotate, node_translate, NULL);
  //gegl_node_link_many (node_path1, node_scale, node_rotate, node_translate, NULL);
  gegl_node_connect (node_translate, "output",  node_over, "aux");
  return node_over;
}

static GeglNode *
draw_pll_overlay (GeglNode* node, GeglNode* source, GeglColor* color, guchar data[])
{
  gint32 i, i1, i2, j, j1, j2;
  guchar n, n1, n2, k;
  GeglNode *node_over, *node_current;

  node_current = source;

  /* first point - smallest one */
  n1 = CUBE_NUMBER * CUBE_NUMBER + 1;
  for (j = 0; j < CUBE_NUMBER; j++) {
    for (i = 0; i < CUBE_NUMBER; i++) {
      k = data[j * CUBE_NUMBER + i];
      if (k > 0 && k < n1) {
        j1 = j2 = j;
        i1 = i2 = i;
        n1 = k;
      }
    }
  }

  if (n1 == CUBE_NUMBER * CUBE_NUMBER + 1) {
    // no points found - no arrows
    return node_current;
  }

  /* following points (up to CUBE_NUMBER) */
  n2 = 0;
  for (n = n1 + 1; n < CUBE_NUMBER * CUBE_NUMBER + 1; n++) {
    for (j = 0; j < CUBE_NUMBER; j++) {
      for (i = 0; i < CUBE_NUMBER; i++) {
        if (n == data[j * CUBE_NUMBER + i]) {
          node_over = draw_pll_arrow (node, color, j2, i2, j, i);
          gegl_node_link_many (node_current, node_over, NULL);
          node_current = node_over;
          j2 = j;
          i2 = i;
          n2 = n;
        }
      }
    }
  }

  if (n2 > n1) {
    node_over = draw_pll_arrow (node, color, j2, i2, j1, i1);
    gegl_node_link_many (node_current, node_over, NULL);
    node_current = node_over;
  }

  return node_current;
}

static void
draw_pll_colors (GeglBuffer* buffer, guchar data[])
{
  gint32 step, i, j;
  GeglNode *node, *node_src, *node_dst, *node_current;
  GeglRectangle rect = { 0, 0, cube_params.layer_size, cube_params.layer_size };

  node = gegl_node_new ();
  node_src = gegl_node_new_child (node, "operation", "gegl:buffer-source", "buffer", buffer, NULL);
  node_dst = gegl_node_new_child (node, "operation", "gegl:write-buffer", "buffer", buffer, NULL);
  node_current = node_src;

  gegl_buffer_set_color (buffer, &rect, render_data.color_black);
  step = render_params.line_width + render_params.cell_size;

  for (j = 0; j < CUBE_NUMBER; j++) {
    for (i = 0; i < CUBE_NUMBER; i++) {
      gegl_rectangle_set (&rect, render_params.line_width + step * i, render_params.line_width + step * j, render_params.cell_size, render_params.cell_size);
      gegl_buffer_set_color (buffer, &rect, render_data.color_top);
    }
  }

  node_current = draw_pll_overlay (node, node_current, render_data.color_line1, &data[0]);
  node_current = draw_pll_overlay (node, node_current, render_data.color_line2, &data[CUBE_NUMBER * CUBE_NUMBER]);
  node_current = draw_pll_overlay (node, node_current, render_data.color_line3, &data[CUBE_NUMBER * CUBE_NUMBER * 2]);

  gegl_node_link_many (node_current, node_dst, NULL);
  gegl_node_process (node_dst);
  g_object_unref (node);
}

/* CUBE */

void
calculate_params (CubeParams* data)
{
  int size_label = 0;
  int size_algorithm = 0;

  if (render_params.font_label.use) {
    size_label = render_params.font_label.size;
  }
  if (render_params.font_algorithm.use) {
    size_algorithm = render_params.font_algorithm.size;
  }

  data->layer_size = render_params.cell_size * CUBE_NUMBER + render_params.line_width * (CUBE_NUMBER + 1);
  data->image_width = data->layer_size + render_params.border_width * 2;
  data->image_height = data->image_width + size_label + size_algorithm;

  data->offset_top = render_params.border_width + size_label;
  data->offset_left = render_params.border_width;

  data->offset_top_label = render_params.border_width;
  data->offset_right_label = cube_params.image_width - render_params.border_width;
  data->offset_top_algorithm = cube_params.image_height - render_params.border_width - render_params.font_algorithm.size;

  data->oll_border_width = 1;
  guint oll_extra = (render_params.oll_side_width + data->oll_border_width) * 2;
  oll_extra += CUBE_NUMBER * 2 - oll_extra % (CUBE_NUMBER * 2);
  data->oll_cell_size = render_params.cell_size - oll_extra / CUBE_NUMBER;
  cube_params.oll_side_width = oll_extra / 2 - data->oll_border_width;
}

static gboolean
parse_combinations ()
{
  guint count;
  gboolean success;

  count = 0;
  if (render_params.render_f2l) {
    count += count_f2l ();
  }
  if (render_params.render_oll) {
    count += count_oll ();
  }
  if (render_params.render_pll) {
    count += count_pll ();
  }
  cube_data = g_array_sized_new (FALSE, TRUE, sizeof (CubeData), count);

  success = TRUE;
  if (success && render_params.render_f2l) {
    success &= add_f2l_data (cube_data);
  }
  if (success && render_params.render_oll) {
    success &= add_oll_data (cube_data);
  }
  if (success && render_params.render_pll) {
    success &= add_pll_data (cube_data);
  }

  if (success && render_params.json_file.use) {
    if (!read_json_data (render_params.json_file.fullname, cube_data, &json_error)) {
      g_printerr ("ERROR: %s\n", json_error->message);
      g_error_free (json_error);
      success = FALSE;
    }
  }

  return success;
}

static void
create_colors ()
{
  render_data.color_black = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_black, 0.0, 0.0, 0.0, 1.0);
  render_data.color_top = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_top, render_params.color_top.r, render_params.color_top.g, render_params.color_top.b, render_params.color_top.a);
  render_data.color_front = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_front, render_params.color_front.r, render_params.color_front.g, render_params.color_front.b, render_params.color_front.a);
  render_data.color_right = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_right, render_params.color_right.r, render_params.color_right.g, render_params.color_right.b, render_params.color_right.a);
  render_data.color_down = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_down, render_params.color_down.r, render_params.color_down.g, render_params.color_down.b, render_params.color_down.a);
  render_data.color_neutral = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_neutral, render_params.color_neutral.r, render_params.color_neutral.g, render_params.color_neutral.b, render_params.color_neutral.a);
  render_data.color_line1 = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_line1, render_params.color_line1.r, render_params.color_line1.g, render_params.color_line1.b, render_params.color_line1.a);
  render_data.color_line2 = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_line2, render_params.color_line2.r, render_params.color_line2.g, render_params.color_line2.b, render_params.color_line2.a);
  render_data.color_line3 = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_line3, render_params.color_line3.r, render_params.color_line3.g, render_params.color_line3.b, render_params.color_line3.a);
  render_data.color_bg = gegl_color_new (NULL);
  gegl_color_set_rgba (render_data.color_bg, render_params.bg_color.r, render_params.bg_color.g, render_params.bg_color.b, render_params.bg_color.a);
}

static void
free_colors ()
{
  g_object_unref (render_data.color_black);
  g_object_unref (render_data.color_top);
  g_object_unref (render_data.color_front);
  g_object_unref (render_data.color_right);
  g_object_unref (render_data.color_down);
  g_object_unref (render_data.color_neutral);
  g_object_unref (render_data.color_line1);
  g_object_unref (render_data.color_line2);
  g_object_unref (render_data.color_line3);
  g_object_unref (render_data.color_bg);
}

static void
create_layers ()
{
  /* create image */
  render_data.image_id = gimp_image_new (cube_params.image_width, cube_params.image_height, GIMP_RGB);

  /* create layers */
  render_data.layer_top = gimp_layer_new (render_data.image_id, "top", cube_params.layer_size, cube_params.layer_size, GIMP_RGBA_IMAGE, 100.0, GIMP_LAYER_MODE_NORMAL);
  render_data.layer_front = gimp_layer_new (render_data.image_id, "front", cube_params.layer_size, cube_params.layer_size, GIMP_RGBA_IMAGE, 100.0, GIMP_LAYER_MODE_NORMAL);
  render_data.layer_right = gimp_layer_new (render_data.image_id, "right", cube_params.layer_size, cube_params.layer_size, GIMP_RGBA_IMAGE, 100.0, GIMP_LAYER_MODE_NORMAL);
  render_data.layer_draw = gimp_layer_new (render_data.image_id, "draw", cube_params.layer_size, cube_params.layer_size, GIMP_RGBA_IMAGE, 100.0, GIMP_LAYER_MODE_NORMAL);

  gimp_image_insert_layer (render_data.image_id, render_data.layer_top, 0, 0);
  gimp_image_insert_layer (render_data.image_id, render_data.layer_front, 0, 0);
  gimp_image_insert_layer (render_data.image_id, render_data.layer_right, 0, 0);
  gimp_image_insert_layer (render_data.image_id, render_data.layer_draw, 0, 0);

  /* create GEGL buffer for each layer */
  render_data.buffer_top = gimp_drawable_get_buffer (render_data.layer_top);
  render_data.buffer_front = gimp_drawable_get_buffer (render_data.layer_front);
  render_data.buffer_right = gimp_drawable_get_buffer (render_data.layer_right);
  render_data.buffer_draw = gimp_drawable_get_buffer (render_data.layer_draw);

  /* create GEGL graph */
  render_data.node = gegl_node_new ();

  /* create SVG path for PLL arrows */
  gdouble line_width = render_params.line_width;
  gdouble half_width = line_width / 2;
  gdouble length = (CUBE_NUMBER - 1) * (render_params.cell_size + render_params.line_width);
  gdouble top_length = length / 2;
  gdouble bottom_length = length / 2 - line_width;
  gdouble head_height = line_width * 8.0;
  gdouble wing_angle = 20.0 * M_PI / 180;
  gdouble wing_width = head_height * sin (wing_angle);
  gdouble tip_height = half_width / tan (wing_angle);
  gdouble tip_width = tip_height * sin (wing_angle);
  gdouble corner_radius = 4.0;
  gdouble corner_bezier = corner_radius * 0.55;
  gdouble tip_bezier = tip_width * 0.4;
  gdouble inner_height = (wing_width - line_width - half_width) / tan (wing_angle);

  render_data.path_arrow = gegl_path_new ();
  gegl_path_append (render_data.path_arrow, 'M', 0.0, 0.0);
  gegl_path_append (render_data.path_arrow, 'm', 0.0, -top_length + tip_height - tip_width);
  /* tip left radius + left wing */
  gegl_path_append (render_data.path_arrow, 'C', -tip_bezier, -top_length + tip_height - tip_width,
                               -tip_width, -top_length + tip_height - tip_bezier,
                               -tip_width, -top_length + tip_height);
  gegl_path_append (render_data.path_arrow, 'L', -wing_width, -top_length + head_height);
  //
  gegl_path_append (render_data.path_arrow, 'L', -wing_width + line_width, -top_length + head_height);
  gegl_path_append (render_data.path_arrow, 'L', -half_width, -top_length + head_height - inner_height);
  /* line with rounded bottom corners */
  gegl_path_append (render_data.path_arrow, 'L', -half_width, bottom_length - corner_radius);
  gegl_path_append (render_data.path_arrow, 'C', -half_width, bottom_length - corner_radius + corner_bezier,
                               -corner_bezier, bottom_length,
                               -half_width + corner_radius, bottom_length);
  gegl_path_append (render_data.path_arrow, 'l', line_width - corner_radius - corner_radius, 0.0);
  gegl_path_append (render_data.path_arrow, 'C', corner_bezier, bottom_length,
                               half_width, bottom_length - corner_radius + corner_bezier,
                               half_width, bottom_length - corner_radius);
  gegl_path_append (render_data.path_arrow, 'L', half_width, -top_length + head_height - inner_height);
  //
  gegl_path_append (render_data.path_arrow, 'L', wing_width - line_width, -top_length + head_height);
  gegl_path_append (render_data.path_arrow, 'L', wing_width, -top_length + head_height);
  /* right wing + tip right radius */
  gegl_path_append (render_data.path_arrow, 'L', tip_width, -top_length + tip_height);
  gegl_path_append (render_data.path_arrow, 'C', tip_width, -top_length + tip_height - tip_bezier,
                               tip_bezier, -top_length + tip_height - tip_width,
                               0.0, -top_length + tip_height - tip_width);
  gegl_path_append (render_data.path_arrow, 'z');
}

static void
free_layers ()
{
  g_object_unref (render_data.path_arrow);

  /* release GEGL nodes and buffers */
  g_object_unref (render_data.node);
  g_object_unref (render_data.buffer_top);
  g_object_unref (render_data.buffer_front);
  g_object_unref (render_data.buffer_right);
  g_object_unref (render_data.buffer_draw);

  /* remove temporary layers */
  gimp_image_remove_layer (render_data.image_id, render_data.layer_top);
  gimp_image_remove_layer (render_data.image_id, render_data.layer_front);
  gimp_image_remove_layer (render_data.image_id, render_data.layer_right);
  gimp_image_remove_layer (render_data.image_id, render_data.layer_draw);
}

/* GEGL graph

                  ┌───────────────┐
                  │     source    │
                  └───────┬───────┘
                          │
                  ┌───────┴───────┐        ┌─────────────┐    ┌──────────┐    ┌────────┐
                  │ line1 overlay │<──aux──┤  translate  ├<───┤  rotate  ├<───┤  path  │
                  └───────┬───────┘        └─────────────┘    └──────────┘    └────────┘
                          │
                  ┌───────┴───────┐        ┌─────────────┐    ┌──────────┐    ┌────────┐
                  │ line2 overlay │<──aux──┤  translate  ├<───┤  rotate  ├<───┤  path  │
                  └───────┬───────┘        └─────────────┘    └──────────┘    └────────┘
                          │
┌─────────────┐           │
│ background  │           │
└──────┬──────┘           │
       │                  │
┌──────┴──────┐   ┌───────┴───────┐
│    tile     │   │   translate   │
└──────┬──────┘   └───────┬───────┘
       │                  │
┌──────┴──────┐   ┌───────┴───────┐        ┌─────────────┐    ┌─────────┐
│    crop     │   │ text1 overlay ├<──aux──┤  translate  ├<───┤  text1  │
└──────┬──────┘   └───────┬───────┘        └─────────────┘    └─────────┘
       │                  │
       │          ┌───────┴───────┐        ┌─────────────┐    ┌─────────┐
       │          │ text2 overlay ├<──aux──┤  translate  ├<───┤  text2  │
       │          └───────┬───────┘        └─────────────┘    └─────────┘
┌──────┴──────┐           │
│   overlay   ├<────aux───┘
└──────┬──────┘
       │
┌──────┴──────┐
│ destination │
└─────────────┘

*/

static gint32 render_cube (void)
{
  gint32 layer_cube;
  GeglBuffer *buffer_cube;
  GeglBuffer *buffer_bg = NULL;
  GeglNode *node_src, *node_src_translate, *node_dst;
  GeglNode *node_over_text1, *node_over_text2, *node_text1, *node_text2, *node_translate_text1, *node_translate_text2;
  GeglNode *node_bg, *node_tile, *node_crop, *node_over_bg;
  GeglNode *node_current;
  gint text1_width, text2_width;

  calculate_params (&cube_params);
  if (!parse_combinations ()) {
    return -1;
  }
  create_colors ();
  create_layers ();

  /* create GEGL nodes */
  node_src = gegl_node_new_child (render_data.node, "operation", "gegl:buffer-source",
                      "buffer", render_data.buffer_draw, NULL);
  node_dst = gegl_node_new_child (render_data.node, "operation", "gegl:write-buffer", NULL);
  node_src_translate = gegl_node_new_child (render_data.node, "operation", "gegl:translate",
                      "x", (gdouble)cube_params.offset_left,
                      "y", (gdouble)cube_params.offset_top,
                      NULL);
  node_over_text1 = gegl_node_new_child (render_data.node, "operation", "gegl:over", NULL);
  node_over_text2 = gegl_node_new_child (render_data.node, "operation", "gegl:over", NULL);
  node_text1 = gegl_node_new_child (render_data.node, "operation", "gegl:text",
                      "font", render_params.font_algorithm.fullname,
                      "size", (gdouble) render_params.font_algorithm.size,
                      "color", gegl_color_new ("black"),
                      NULL);
  node_text2 = gegl_node_new_child (render_data.node, "operation", "gegl:text",
                      "font", render_params.font_label.fullname,
                      "size", (gdouble) render_params.font_label.size,
                      "color", gegl_color_new ("black"),
                      NULL);
  node_translate_text1 = gegl_node_new_child (render_data.node, "operation", "gegl:translate",
                      "y", (gdouble)(cube_params.offset_top_algorithm),
                      NULL);
  node_translate_text2 = gegl_node_new_child (render_data.node, "operation", "gegl:translate",
                      "y", (gdouble)(cube_params.offset_top_label),
                      NULL);
  node_tile = gegl_node_new_child (render_data.node, "operation", "gegl:tile", NULL);
  node_crop = gegl_node_new_child (render_data.node, "operation", "gegl:crop",
                      "width", (gdouble)cube_params.image_width,
                      "height", (gdouble)cube_params.image_height,
                      NULL);
  node_over_bg = gegl_node_new_child (render_data.node, "operation", "gegl:over", NULL);

  /* create background */
  switch (render_params.bg_type) {
    case BG_TRANSPARENT:
      node_bg = gegl_node_new_child (render_data.node, "operation", "gegl:over", NULL);
      break;
    case BG_COLOR:
      node_bg = gegl_node_new_child (render_data.node, "operation", "gegl:color",
                      "value", render_data.color_bg,
                      NULL);
      break;
    case BG_CUSTOM:
      node_bg = gegl_node_new_child (render_data.node, "operation", "gegl:load",
                      "path", render_params.bg_file.fullname,
                      NULL);
      break;
    default:
      buffer_bg = create_bg_buffer ();
      node_bg = gegl_node_new_child (render_data.node, "operation", "gegl:buffer-source",
                      "buffer", buffer_bg, NULL);
      break;
  }

  /* compose GEGL graph */
  gegl_node_link_many (node_bg, node_tile, node_crop, node_over_bg, node_dst, NULL);
  gegl_node_link_many (node_text1, node_translate_text1, NULL);
  gegl_node_link_many (node_text2, node_translate_text2, NULL);
  gegl_node_connect (node_translate_text1, "output",  node_over_text1, "aux");
  gegl_node_connect (node_translate_text2, "output",  node_over_text2, "aux");
  gegl_node_link_many (node_src, node_src_translate, NULL);

  node_current = node_src_translate; /* node_src_translate -> ... text ...  -> node_over_bg */
  if (render_params.font_algorithm.use) {
    gegl_node_link_many (node_current, node_over_text1, NULL);
    node_current = node_over_text1;
  }
  if (render_params.font_label.use) {
    gegl_node_link_many (node_current, node_over_text2, NULL);
    node_current = node_over_text2;
  }
  gegl_node_connect (node_current, "output",  node_over_bg, "aux");

  /* iterate cube combinations */
  for (int i = 0; i < cube_data->len; i++) {
    CubeData cube = g_array_index (cube_data, CubeData, i);
    if (cube.type == CUBE_F2L) {
      // F2L render uses layer, not buffer, so buffer has to be re-created (don't fully get it, though)
      g_object_unref (render_data.buffer_draw);
      render_data.buffer_draw = gimp_drawable_get_buffer (render_data.layer_draw);
      gegl_node_set (node_src, "buffer", render_data.buffer_draw, NULL);
      //gegl_buffer_clear (render_data.buffer_draw, ...); // TODO: TRY IT

      render_f2l_cube (&cube.combinations.data[0]);
    } else if (cube.type == CUBE_OLL) {
      draw_oll_colors (render_data.buffer_draw, &cube.combinations.data[0]);
    } else if (cube.type == CUBE_PLL) {
      draw_pll_colors (render_data.buffer_draw, &cube.combinations.data[0]);
    }

    /* layer to draw final image (background + image itself + overlays) */
    layer_cube = gimp_layer_new (render_data.image_id, cube.label, cube_params.image_width, cube_params.image_height, GIMP_RGBA_IMAGE, 100.0, GIMP_LAYER_MODE_NORMAL);
    gimp_image_insert_layer (render_data.image_id, layer_cube, 0, 0);
    buffer_cube = gimp_drawable_get_buffer (layer_cube);

    /* GEGL graph adjustments and rendering */
    gegl_node_set (node_text1, "string", cube.algorithm, NULL);
    gegl_node_set (node_text2, "string", cube.label, NULL);
    gegl_node_get (node_text1, "width", &text1_width, NULL);
    gegl_node_get (node_text2, "width", &text2_width, NULL);
    gegl_node_set (node_translate_text1, "x", text1_width > cube_params.image_width ? 0.0 : (gdouble)(cube_params.image_width - text1_width) / 2, NULL);
    gegl_node_set (node_translate_text2, "x", (gdouble)(cube_params.offset_right_label - text2_width), NULL);
    gegl_node_set (node_dst, "buffer", buffer_cube, NULL);
    gegl_node_process (node_dst);

    g_object_unref (buffer_cube);
    gimp_item_set_visible (layer_cube, FALSE);
  }

  if (buffer_bg) g_object_unref (buffer_bg);
  free_layers ();
  free_colors ();
  g_array_free (cube_data, TRUE);

  gimp_display_new (render_data.image_id);
  return render_data.image_id;
}

/* PLUG-IN */

static void query(void)
{
  static const GimpParamDef params[] = {
    { GIMP_PDB_INT32,    "run-mode", "Run mode { GIMP_RUN_INTERACTIVE (0), GIMP_RUN_NONINTERACTIVE (1), GIMP_RUN_WITH_LAST_VALS (2) }" },
    //{ GIMP_PDB_IMAGE,    "image",    "Input image"    },
    //{ GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32,  "cell-size",        HELP_CELL_SIZE        },
    { GIMP_PDB_INT32,  "line-width",       HELP_LINE_WIDTH       },
    { GIMP_PDB_INT32,  "oll-side-width",    HELP_OLL_WIDTH        },
    //{ GIMP_PDB_INT32,  "border-width",  HELP_BORDER_WIDTH  },
    { GIMP_PDB_COLOR,  "top-color",        HELP_TOP_COLOR        },
    { GIMP_PDB_COLOR,  "front-color",      HELP_FRONT_COLOR      },
    { GIMP_PDB_COLOR,  "down-color",       HELP_DOWN_COLOR       },
    { GIMP_PDB_COLOR,  "right-color",      HELP_RIGHT_COLOR      },
    { GIMP_PDB_COLOR,  "neutral-color",    HELP_NEUTRAL_COLOR    },
    { GIMP_PDB_COLOR,  "line1-color",      HELP_LINE1_COLOR      },
    { GIMP_PDB_COLOR,  "line2-color",      HELP_LINE2_COLOR      },
    { GIMP_PDB_INT32,  "render-f2l",       HELP_RENDER_F2L       },
    { GIMP_PDB_INT32,  "render-oll",       HELP_RENDER_OLL       },
    { GIMP_PDB_INT32,  "render-pll",       HELP_RENDER_PLL       },
    { GIMP_PDB_INT32,  "render-file",      HELP_RENDER_FILE      },
    { GIMP_PDB_STRING, "filename",         HELP_FILE_ALGORITHMS  },
    { GIMP_PDB_INT32,  "render-label",     HELP_RENDER_LABEL     },
    { GIMP_PDB_INT32,  "render-algorithm", HELP_RENDER_ALGORITHM },
  };

  static const GimpParamDef return_vals[] = {
    { GIMP_PDB_STATUS, "status", "Output status" },
    { GIMP_PDB_IMAGE,  "image",  "Output image"  },
  };

  gimp_install_procedure (
    PLUG_IN_PROC,
    "Rubik's Cube combinations renderer",
    _("Generates images for Rubik's Cube combinations"),
    "Dzmitry Safin",
    "Copyright © Dzmitry Safin",
    "2023",
    "Rubik's Cube",
    "", // "RGB*, GRAY*",
    GIMP_PLUGIN,
    G_N_ELEMENTS (params), G_N_ELEMENTS (return_vals),
    params, return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Toolbox>/Filters/Render");
}

static void run(
  const gchar *name,
  gint         nparams,
  const        GimpParam *param,
  gint        *nreturn_vals,
  GimpParam  **return_vals)
{
  static GimpParam values[2];
  GimpRunMode run_mode;
  gint32 image_id;
  GimpPDBStatusType status;

  //INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals = 2;
  *return_vals = values;
  values[0].type = GIMP_PDB_STATUS;
  values[1].type = GIMP_PDB_IMAGE;

  // image_id = render_test ();
  // values[0].data.d_status = GIMP_PDB_SUCCESS;
  // values[1].data.d_image = image_id;
  // return;

  status = GIMP_PDB_SUCCESS;
  image_id = 0;
  run_mode = param[0].data.d_int32;
  switch (run_mode) {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &render_params);
      if (main_dialog(&render_params)) {
        image_id = render_cube ();
        gimp_set_data (PLUG_IN_PROC, &render_params, sizeof(RenderParams));
      }
      break;
    case GIMP_RUN_NONINTERACTIVE:
      if (nparams != 13) {
        status = GIMP_PDB_CALLING_ERROR;
      } else {
        render_params.cell_size     = param[4].data.d_int32;
        render_params.line_width    = param[5].data.d_int32;
        render_params.color_top     = param[6].data.d_color;
        render_params.color_front   = param[7].data.d_color;
        render_params.color_right   = param[8].data.d_color;
        render_params.color_down    = param[9].data.d_color;
        render_params.color_neutral = param[10].data.d_color;
        render_params.render_f2l        = (gboolean)param[11].data.d_int32;
        render_params.render_oll        = (gboolean)param[12].data.d_int32;
        render_params.render_pll        = (gboolean)param[13].data.d_int32;
        render_params.json_file.use      = (gboolean)param[14].data.d_int32;
        //render_params.json_file.fullname = param[15].data.d_string;

        image_id = render_cube ();
      }
      break;
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &render_params);
      image_id = render_cube ();
      break;
    default:
      status = GIMP_PDB_CALLING_ERROR;
      break;
  }

  if (image_id == -1) {
    status = GIMP_PDB_EXECUTION_ERROR;
  }
  values[0].data.d_status = status;
  values[1].data.d_image = image_id;

  // if (render_params.json_file.fullname != NULL) {
  //   g_free (render_params.json_file.fullname);
  //   render_params.json_file.fullname = NULL;
  // }
  // if (render_params.bg_file.fullname != NULL) {
  //   g_free (render_params.bg_file.fullname);
  //   render_params.bg_file.fullname = NULL;
  // }

  if (run_mode != GIMP_RUN_NONINTERACTIVE) {
    gimp_displays_flush ();
  }
}

GimpPlugInInfo PLUG_IN_INFO = {
  NULL,  // init_proc
  NULL,  // quit_proc
  query, // query_proc
  run    // run_proc
};

MAIN()

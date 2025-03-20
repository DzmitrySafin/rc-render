#include "rc-render-main.h"

static GArray *cube_data;
static GError *json_error = NULL;

static gboolean
read_json_pll (JsonReader* reader, const gchar* member_name, guchar data[])
{
  gint count;
  guint i;
  const gchar *line;

  /* read given side node. make sure it is array with sufficient amount of elements. */
  if (!json_reader_read_member (reader, member_name)) {
    /* not used overlay */
    for (i = 0; i < CUBE_NUMBER; i++) {
      parse_pll_line (NULL, &data[CUBE_NUMBER * i]);
    }
    json_reader_end_member (reader);
    return TRUE;
  }
  count = json_reader_is_array (reader) ? json_reader_count_elements (reader) : 0;
  if (count != CUBE_NUMBER) {
    return FALSE;
  }

  for (i = 0; i < CUBE_NUMBER; i++) {
    if (!json_reader_read_element (reader, i)) {
      return FALSE;
    }
    line = json_reader_get_string_value (reader);
    json_reader_end_element (reader);

    if (!parse_pll_line (line, &data[CUBE_NUMBER * i])) {
      return FALSE;
    }
  }

  json_reader_end_member (reader);
  return TRUE;
}

static gboolean
read_json_oll (JsonReader* reader, const gchar* member_name, guchar data[])
{
  gint count;
  guint i;
  const gchar *line;

  /* read given side node. make sure it is array with sufficient amount of elements. */
  if (!json_reader_read_member (reader, member_name)) {
    return FALSE;
  }
  count = json_reader_is_array (reader) ? json_reader_count_elements (reader) : 0;
  if (count != CUBE_NUMBER_EX) {
    return FALSE;
  }

  for (i = 0; i < CUBE_NUMBER_EX; i++) {
    if (!json_reader_read_element (reader, i)) {
      return FALSE;
    }
    line = json_reader_get_string_value (reader);
    json_reader_end_element (reader);

    if (!parse_oll_line (line, &data[CUBE_NUMBER_EX * i])) {
      return FALSE;
    }
  }

  json_reader_end_member (reader);
  return TRUE;
}

static gboolean
read_json_f2l (JsonReader* reader, const gchar* member_name, guchar data[])
{
  gint count;
  guint i;
  const gchar *line;

  /* read given side node. make sure it is array with sufficient amount of elements. */
  if (!json_reader_read_member (reader, member_name)) {
    return FALSE;
  }
  count = json_reader_is_array (reader) ? json_reader_count_elements (reader) : 0;
  if (count != CUBE_NUMBER) {
    return FALSE;
  }

  for (i = 0; i < CUBE_NUMBER; i++) {
    if (!json_reader_read_element (reader, i)) {
      return FALSE;
    }
    line = json_reader_get_string_value (reader);
    json_reader_end_element (reader);

    if (!parse_f2l_line (line, &data[CUBE_NUMBER * i])) {
      return FALSE;
    }
  }

  json_reader_end_member (reader);
  return TRUE;
}

/* COMMON */

static gboolean
read_json_content (JsonReader *reader)
{
  guint precount, i;
  gint  count, n;
  gsize length;
  const gchar *type;

  /* read 'data' node. make sure it is non-empty array. */
  if (!json_reader_read_member (reader, "data")) {
    g_set_error (&json_error, 0, 0, "Cannot read 'data' node from JSON file.");
    return FALSE;
  }
  count = json_reader_is_array (reader) ? json_reader_count_elements (reader) : 0;
  if (count <= 0) {
    g_set_error (&json_error, 0, 0, "Node 'data' must be non-empty array.");
    return FALSE;
  }

  /* TODO: create array with pre-allocated size */
  precount = cube_data->len;
  g_array_set_size (cube_data, cube_data->len + count);

  /* read 'data' array of cube objects */
  for (n = 0; n < count; n++) {
    CubeData *cube = &g_array_index (cube_data, CubeData, precount + n);
    cube->index = n;

    if (!json_reader_read_element (reader, n)) {
      g_set_error (&json_error, 0, 0, "Cannot read cube data object [%d].", n);
      break;
    }

    /* schema type */
    if (!json_reader_read_member (reader, "type")) {
      g_set_error (&json_error, 0, 0, "Cannot read 'type' node for the cube data object [%d].", n);
      break;
    }
    type = json_reader_get_string_value (reader);
    json_reader_end_member (reader);

    /* schema label */
    if (!json_reader_read_member (reader, "label")) {
      g_set_error (&json_error, 0, 0, "Cannot read 'label' node for the cube data object [%d].", n);
      break;
    }
    length = g_strlcpy (cube->label, json_reader_get_string_value (reader), BUFFER_SIZE_MIDDLE);
    json_reader_end_member (reader);
    if (length >= BUFFER_SIZE_MIDDLE) {
      g_set_error (&json_error, 0, 0, "Value of the 'label' node exceeds maximum length [%d] for the cube data object [%d].", BUFFER_SIZE_MIDDLE, n);
      break;
    }

    /* schema algorithm */
    if (!json_reader_read_member (reader, "algorithm")) {
      g_set_error (&json_error, 0, 0, "Cannot read 'algorithm' node for the cube data object [%d].", n);
      break;
    }
    length = g_strlcpy (cube->algorithm, json_reader_get_string_value (reader), BUFFER_SIZE_LONG);
    json_reader_end_member (reader);
    if (length >= BUFFER_SIZE_LONG) {
      g_set_error (&json_error, 0, 0, "Value of the 'algorithm' node exceeds maximum length [%d] for the cube data object [%d].", BUFFER_SIZE_LONG, n);
      break;
    }

    if (strcasecmp (type, "F2L") == 0) {
      cube->type = CUBE_F2L;
      static gchar *node_schema[] = { "schema-top", "schema-front", "schema-right" };
      for (i = 0; i < 3; i++) {
        if (!read_json_f2l (reader, node_schema[i], &cube->combinations.data[CUBE_NUMBER * CUBE_NUMBER * i])) {
          g_set_error (&json_error, 0, 0, "Cannot read '%s' node for the cube data object [%d].", node_schema[i], n);
          break;
        }
      }
    } else if (strcasecmp (type, "OLL") == 0) {
      cube->type = CUBE_OLL;
      static gchar *node_schema = "schema";
      if (!read_json_oll (reader, node_schema, &cube->combinations.data[CUBE_NUMBER_EX * CUBE_NUMBER_EX * 0])) {
        g_set_error (&json_error, 0, 0, "Cannot read '%s' node for the cube data object [%d].", node_schema, n);
        break;
      }
    } else if (strcasecmp (type, "PLL") == 0) {
      cube->type = CUBE_PLL;
      static gchar *node_schema[] = { "schema-1", "schema-2", "schema-3" };
      for (i = 0; i < 3; i++) {
        if (!read_json_pll (reader, node_schema[i], &cube->combinations.data[CUBE_NUMBER * CUBE_NUMBER * i])) {
          g_set_error (&json_error, 0, 0, "Cannot read '%s' node for the cube data object [%d].", node_schema[i], n);
          break;
        }
      }
    } else {
      g_set_error (&json_error, 0, 0, "Invalid value for the 'type' node for the cube data object [%d].", n);
      break;
    }

    json_reader_end_element (reader);
  }

  /* in case of reading error above we should call json_reader_end_element first
     but it is ok, since we will not read file anymore */
  json_reader_end_member (reader); // 'data'
  return n == count;
}

gboolean
read_json_data (gchar* filename, GArray* combinations, GError** error)
{
  JsonParser *parser;
  JsonNode   *root;
  JsonReader *reader;
  gboolean    success;

  g_assert (filename != NULL);
  g_assert (combinations != NULL);
  g_assert (error != NULL);
  g_assert (*error == NULL);
  cube_data = combinations;

  parser = json_parser_new ();
  if (!json_parser_load_from_file (parser, filename, &json_error)) {
    g_set_error (error, 0, 0, "Cannot create JSON parser.");
    g_object_unref (parser);
    return FALSE;
  }

  root = json_parser_get_root (parser);
  reader = root == NULL ? NULL : json_reader_new (root);
  if (reader == NULL) {
    g_set_error (error, 0, 0, "Cannot create JSON reader for the root node.");
    g_object_unref (parser);
    return FALSE;
  }

  success = read_json_content (reader);
  if (!success) {
    *error = json_error;
  }

  g_object_unref (reader);
  g_object_unref (parser);
  return success;
}

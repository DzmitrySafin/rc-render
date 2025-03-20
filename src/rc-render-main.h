#ifndef __rc_render_main_h__
#define __rc_render_main_h__

#include <stdio.h>
#ifdef _WIN64
#include <windows.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#pragma clang diagnostic pop

#include <libintl.h>
#define _(String)            gettext (String)
#define gettext_noop(String) String
#define N_(String)           gettext_noop (String)

#include <json-glib/json-glib.h>

#define PLUG_IN_PROC   "plug-in-rc-renderer"
#define PLUG_IN_BINARY "rc-renderer"
#define PLUG_IN_ROLE   "rc-renderer-c"
#define CUBE_NUMBER    3 /* cube 3x3 */
#define CUBE_NUMBER_EX (CUBE_NUMBER + 2) /* including left/right sides for OLL */

#define BUFFER_SIZE_SHORT  16
#define BUFFER_SIZE_MIDDLE 32
#define BUFFER_SIZE_LONG   64

#define LEFT_1(x) ( (x) | (x) >> 1 )
#define LEFT_2(x) ( LEFT_1(x) | (x) >> 2 )
#define LEFT_4(x) ( LEFT_2(x) | (x) >> 4 )
#define LEFT_8(x) ( LEFT_4(x) | (x) >> 8 )
#define LEFT_16(x) ( LEFT_8(x) | (x) >> 16 )
#define LEFT_POWER2(x) ( LEFT_16(x - 1) + 1 )

#define HELP_CELL_SIZE        "Cube single cell width and height"
#define HELP_LINE_WIDTH       "Cube lines width (space between cells)"
#define HELP_OLL_WIDTH        "Width of the side part on OLL schema"
//#define HELP_BORDER_WIDTH
#define HELP_TOP_COLOR        "Cube top side color"
#define HELP_FRONT_COLOR      "Cube front side color"
#define HELP_DOWN_COLOR       "Cube down side color"
#define HELP_RIGHT_COLOR      "Cube right side color"
#define HELP_NEUTRAL_COLOR    "Cube neutral color"
#define HELP_LINE1_COLOR      "Cube PLL primary arrow color"
#define HELP_LINE2_COLOR      "Cube PLL secondary arrow color"
#define HELP_RENDER_F2L       "Render F2L algorithms"
#define HELP_RENDER_OLL       "Render OLL algorithms"
#define HELP_RENDER_PLL       "Render PLL algorithms"
#define HELP_RENDER_FILE      "Render algorithms from the file"
#define HELP_FILE_ALGORITHMS  "JSON file with custom algorithms"
#define HELP_RENDER_LABEL     "Render label in the upper right corner"
#define HELP_RENDER_ALGORITHM "Render algorithm in the bottom of the image"

/* TYPES */

/* cube colors encoding */
typedef enum
{
  CUBE_COLOR_NEUTRAL, /* 0 - neutral color    (gray)   */
  CUBE_COLOR_TOP,     /* T - top side color   (yellow) */
  CUBE_COLOR_FRONT,   /* F - front side color (red)    */
  CUBE_COLOR_RIGHT,   /* R - right side color (green)  */
  CUBE_COLOR_DOWN     /* D - down side color  (white)  */
} CubeColorType;

/* combinations to be rendered */
typedef enum
{
  CUBE_NONE = 0,
  CUBE_F2L  = 2, /* process F2L schemes */
  CUBE_OLL  = 4, /* process OLL schemes */
  CUBE_PLL  = 8  /* process PLL schemes */
} CubeCombinationType;

typedef enum
{
  BG_DEFAULT     = 0,
  BG_TRANSPARENT = 1,
  BG_COLOR       = 2,
  BG_CUSTOM      = 3
} BackgroundType;

struct _RenderFont
{
  gboolean use;
  gint     size;
  gchar    fullname[BUFFER_SIZE_LONG];
};
typedef struct _RenderFont RenderFont;

struct _RenderFile
{
  gboolean use;
  gchar    fullname[PATH_MAX];
};
typedef struct _RenderFile RenderFile;

struct _RenderParams
{
  /* size parameters */
  guint cell_size;
  guint line_width;
  guint oll_side_width;
  guint border_width;
  /* colors */
  GimpRGB color_top;
  GimpRGB color_front;
  GimpRGB color_down;
  GimpRGB color_right;
  GimpRGB color_neutral;
  GimpRGB color_line1;
  GimpRGB color_line2;
  GimpRGB color_line3;
  /* combinations */
  gboolean   render_f2l;
  gboolean   render_oll;
  gboolean   render_pll;
  RenderFile json_file;
  /* text */
  RenderFont font_label;
  RenderFont font_algorithm;
  /* different */
  BackgroundType bg_type;
  GimpRGB bg_color;
  RenderFile bg_file;
};
typedef struct _RenderParams RenderParams;

struct _CubeParams
{
  guint layer_size;   /* cube square size */
  guint image_width;  /* real image size */
  guint image_height; /* including texts and borders */

  guint offset_top;          /* cube layer offset */
  guint offset_left;
  guint offset_top_label;     /* label text offset */
  guint offset_right_label;
  guint offset_top_algorithm; /* algorithm text offset */

  guint oll_cell_size;    /* calculated cell size for OLL schema */
  guint oll_border_width; /* outer line width for OLL schema */
  guint oll_side_width;   /* calculated side width for OLL schema */
};
typedef struct _CubeParams CubeParams;

struct _RenderData
{
  GeglColor *color_black;
  GeglColor *color_top;
  GeglColor *color_front;
  GeglColor *color_right;
  GeglColor *color_down;
  GeglColor *color_neutral;
  GeglColor *color_line1;
  GeglColor *color_line2;
  GeglColor *color_line3;
  GeglColor *color_bg;

  gint32 image_id;
  gint32 layer_top;
  gint32 layer_front;
  gint32 layer_right;
  gint32 layer_draw; /* render F2L cube or draw OLL/PLL squares */

  GeglBuffer *buffer_top;
  GeglBuffer *buffer_front;
  GeglBuffer *buffer_right;
  GeglBuffer *buffer_draw;

  GeglNode *node; /* main node for GEGL graph */
  GeglPath *path_arrow;
};
typedef struct _RenderData RenderData;

union _CubeDataCombinations
{
  guchar data[LEFT_POWER2(CUBE_NUMBER*CUBE_NUMBER*3)]; /* bytes used: F2L - 27, OLL - 25, PLL - ? */
  gchar *text[3][CUBE_NUMBER_EX]; /* data hardcoded in text in combinations.c */
};
typedef union  _CubeDataCombinations CubeDataCombinations;

struct _CubeData
{
  gint index;               /* object index in the file */
  CubeCombinationType type; /* F2L / OLL / PLL */
  gchar  label[BUFFER_SIZE_MIDDLE];
  gchar  algorithm[BUFFER_SIZE_LONG];
  CubeDataCombinations combinations;
};
typedef struct _CubeData CubeData;

/* PROCEDURES */

void
calculate_params (CubeParams* data);

gboolean
main_dialog(RenderParams* params);

gboolean
parse_f2l_line (const gchar* line, guchar data[]);

gboolean
parse_oll_line (const gchar* line, guchar data[]);

gboolean
parse_pll_line (const gchar* line, guchar data[]);

gboolean
read_json_data (gchar* filename, GArray* combinations, GError** err);

guint
count_f2l ();

guint
count_oll ();

guint
count_pll ();

gboolean
add_f2l_data (GArray* combinations);

gboolean
add_oll_data (GArray* combinations);

gboolean
add_pll_data (GArray* combinations);

GeglBuffer *
create_bg_buffer ();

#endif /* __rc_render_main_h__ */

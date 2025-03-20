#!/usr/bin/env python
# chmod ugo+x *.py # must be executable for GIMP

#import math
from gimpfu import *

def add_layer(image, name, transparent):
    layer = gimp.Layer(image, name, image.width, image.height, RGBA_IMAGE, 100, NORMAL_MODE)
    if transparent:
        layer.fill(TRANSPARENT_FILL)
    else:
        layer.fill(BACKGROUND_FILL)
    layer.visible = False
    image.add_layer(layer, 0)
    return layer

def draw_mesh(layer, cell_size, line_width):
    width = layer.width
    height = layer.height
    d = line_width + cell_size
    for n in range(line_width):
        w = width - n - 1
        h = height - n - 1
        # perimeter - rectangle
        points = [n, n, w, n, w, h, n, h, n, n]
        pdb.gimp_pencil(layer, len(points), points)
        # two vertical lines
        points = [d + n, n, d + n, h]
        pdb.gimp_pencil(layer, len(points), points)
        points = [d * 2 + n, n, d * 2 + n, h]
        pdb.gimp_pencil(layer, len(points), points)
        # two horizontal lines
        points = [n, d + n, w, d + n]
        pdb.gimp_pencil(layer, len(points), points)
        points = [n, d * 2 + n, w, d * 2 + n]
        pdb.gimp_pencil(layer, len(points), points)

def fill_colors(scheme, layer_front, layer_left, layer_top):
    front = scheme[0].split("-")
    left = scheme[1].split("-")
    top = scheme[2].split("-")
    w = layer_front.width / 6
    # type of fill { FILL-FOREGROUND (0), FILL-BACKGROUND (1), FILL-WHITE (2), FILL-TRANSPARENT (3), FILL-PATTERN (4) }
    # RED color (TODO: get from variable)
    gimp.set_foreground(255, 0, 0, 1.0)
    for n in range(9):
        x = (n % 3) * 2 + 1
        y = (n // 3) * 2 + 1
        if (front[n] == 'R'):
            pdb.gimp_drawable_edit_bucket_fill(layer_front, 0, w * x, w * y)
        if (left[n] == 'R'):
            pdb.gimp_drawable_edit_bucket_fill(layer_left, 0, w * x, w * y)
        if (top[n] == 'R'):
            pdb.gimp_drawable_edit_bucket_fill(layer_top, 0, w * x, w * y)
    # GREEN color (TODO: get from variable)
    gimp.set_foreground(0, 255, 0, 1.0)
    for n in range(9):
        x = (n % 3) * 2 + 1
        y = (n // 3) * 2 + 1
        if (front[n] == 'G'):
            pdb.gimp_drawable_edit_bucket_fill(layer_front, 0, w * x, w * y)
        if (left[n] == 'G'):
            pdb.gimp_drawable_edit_bucket_fill(layer_left, 0, w * x, w * y)
        if (top[n] == 'G'):
            pdb.gimp_drawable_edit_bucket_fill(layer_top, 0, w * x, w * y)
    # WHITE color (TODO: get from variable)
    gimp.set_foreground(255, 255, 255, 1.0)
    for n in range(9):
        x = (n % 3) * 2 + 1
        y = (n // 3) * 2 + 1
        if (front[n] == 'W'):
            pdb.gimp_drawable_edit_bucket_fill(layer_front, 0, w * x, w * y)
        if (left[n] == 'W'):
            pdb.gimp_drawable_edit_bucket_fill(layer_left, 0, w * x, w * y)
        if (top[n] == 'W'):
            pdb.gimp_drawable_edit_bucket_fill(layer_top, 0, w * x, w * y)
    # GRAY color - FILL-BACKGROUND
    for n in range(9):
        x = (n % 3) * 2 + 1
        y = (n // 3) * 2 + 1
        if (front[n] == '0'):
            pdb.gimp_drawable_edit_bucket_fill(layer_front, 1, w * x, w * y)
        if (left[n] == '0'):
            pdb.gimp_drawable_edit_bucket_fill(layer_left, 1, w * x, w * y)
        if (top[n] == '0'):
            pdb.gimp_drawable_edit_bucket_fill(layer_top, 1, w * x, w * y)

def render(img_size, cell_size, line_width, gray_color, front_color, left_color, top_color):
    #msg = "Image size: {0}\nCell size: {1}\nLine width: {2}".format(img_size, cell_size, line_width)
    #pdb.gimp_message(msg)

    image = gimp.Image(img_size, img_size, RGB)
    # apply some settings
    gimp.set_background(gray_color)
    gimp.set_foreground(0, 0, 0, 1.0)
    pdb.gimp_context_set_brush_size(1.0)
    # create layers for three sides + layer for the cube
    layer_front = add_layer(image, "front", False)
    layer_left = add_layer(image, "left", False)
    layer_top = add_layer(image, "top", False)
    layer_box = add_layer(image, "box", True)
    # draw lines/cells
    draw_mesh(layer_front, cell_size, line_width)
    draw_mesh(layer_left, cell_size, line_width)
    draw_mesh(layer_top, cell_size, line_width)
    #
    #scheme = ["0-0-R-R-R-0-R-R-0", "W-0-0-0-G-G-0-G-G", "0-0-0-R-0-0-0-0-G"]
    scheme = ["0-0-W-R-R-0-R-R-0", "G-0-0-0-G-G-0-G-G", "0-G-0-0-0-0-0-0-R"]
    fill_colors(scheme, layer_front, layer_left, layer_top)
    # draw cube
    pdb.plug_in_map_object(image, layer_box,
        2, # type of mapping - box
        0.50, 0.50, 2.00, # position of viewpoint
        0.50, 0.46, 0.00, # object position
        1.00, 0.00, 0.00, # first axis of object
        0.00, 1.00, 0.00, # second axis of object
        35.00, 45.00, 0.00, # rotation about X/Y/Z axis in degrees
        2, # type of lightsource - none
        (0, 0, 0), # lightsource color
        0.00, 0.00, 0.00, # lightsource position
        0.00, 0.00, 0.00, # lightsource direction
        0.30,  # material ambient intensity (0..1)
        1.00,  # material diffuse intensity (0..1)
        0.50,  # material diffuse reflectivity (0..1)
        0.50,  # material specular reflectivity (0..1)
        27.00, # material highlight (0..->)
        True,  # apply antialiasing
        False, # tile source image
        False, # create a new image
        True,  # make background transparent
        0.00,  # sphere/cylinder radius
        0.60, 0.60, 0.60, # box size (scale, 0..->)
        0.00,  # cylinder length (0..->)
        layer_front, # box front face
        None,        # box back face
        layer_top,   # box top face
        None,        # box bottom face
        layer_left,  # box left face
        None,        # box right face
        None, None)  # cylinder top/bottom face
    gimp.Display(image)

register(
    "python-rc-renderer",
    "Rubik's Cube renderer",
    "Generates images for Rubik's Cube combinations",
    "Dzmitry Safin",
    "Copyright @ Dzmitry Safin",
    "2022",
    "<Toolbox>/Filters/Map/Rubik's Cube Python",
    "",
    [
        (PF_INT, "img_size", "Image size", 940),
        (PF_INT, "cell_size", "Cell size", 300),
        (PF_INT, "line_width", "Line width", 10),
        (PF_COLOR, "gray_color", "Neutral color", (169, 169, 169, 1.0)),
        (PF_COLOR, "front_color", "Front side", (255, 0, 0, 1.0)),
        (PF_COLOR, "left_color", "Left side", (0, 255, 0, 1.0)),
        (PF_COLOR, "top_color", "Top side", (255, 255, 0, 1.0)),
    ],
    [],
    render)

main()

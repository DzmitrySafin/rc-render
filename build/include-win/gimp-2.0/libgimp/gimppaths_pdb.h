/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimppaths_pdb.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* NOTE: This file is auto-generated by pdbgen.pl */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PATHS_PDB_H__
#define __GIMP_PATHS_PDB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


GIMP_DEPRECATED_FOR(gimp_image_get_vectors)
gchar**  gimp_path_list              (gint32           image_ID,
                                      gint            *num_paths);
GIMP_DEPRECATED_FOR(gimp_image_get_active_vectors)
gchar*   gimp_path_get_current       (gint32           image_ID);
GIMP_DEPRECATED_FOR(gimp_image_set_active_vectors)
gboolean gimp_path_set_current       (gint32           image_ID,
                                      const gchar     *name);
GIMP_DEPRECATED_FOR(gimp_image_remove_vectors)
gboolean gimp_path_delete            (gint32           image_ID,
                                      const gchar     *name);
GIMP_DEPRECATED_FOR(gimp_vectors_stroke_get_points)
gint     gimp_path_get_points        (gint32           image_ID,
                                      const gchar     *name,
                                      gint            *path_closed,
                                      gint            *num_path_point_details,
                                      gdouble        **points_pairs);
GIMP_DEPRECATED_FOR(gimp_vectors_stroke_new_from_points)
gboolean gimp_path_set_points        (gint32           image_ID,
                                      const gchar     *name,
                                      gint             ptype,
                                      gint             num_path_points,
                                      const gdouble   *points_pairs);
GIMP_DEPRECATED_FOR(gimp_edit_stroke_vectors)
gboolean gimp_path_stroke_current    (gint32           image_ID);
GIMP_DEPRECATED_FOR(gimp_vectors_stroke_get_point_at_dist)
gint     gimp_path_get_point_at_dist (gint32           image_ID,
                                      gdouble          distance,
                                      gint            *y_point,
                                      gdouble         *slope);
GIMP_DEPRECATED_FOR(gimp_vectors_get_tattoo)
gint     gimp_path_get_tattoo        (gint32           image_ID,
                                      const gchar     *name);
GIMP_DEPRECATED_FOR(gimp_vectors_set_tattoo)
gboolean gimp_path_set_tattoo        (gint32           image_ID,
                                      const gchar     *name,
                                      gint             tattovalue);
GIMP_DEPRECATED_FOR(gimp_image_get_vectors_by_tattoo)
gchar*   gimp_get_path_by_tattoo     (gint32           image_ID,
                                      gint             tattoo);
GIMP_DEPRECATED_FOR(gimp_vectors_get_linked)
gboolean gimp_path_get_locked        (gint32           image_ID,
                                      const gchar     *name);
GIMP_DEPRECATED_FOR(gimp_vectors_set_linked)
gboolean gimp_path_set_locked        (gint32           image_ID,
                                      const gchar     *name,
                                      gboolean         locked);
GIMP_DEPRECATED_FOR(gimp_vectors_to_selection)
gboolean gimp_path_to_selection      (gint32           image_ID,
                                      const gchar     *name,
                                      GimpChannelOps   op,
                                      gboolean         antialias,
                                      gboolean         feather,
                                      gdouble          feather_radius_x,
                                      gdouble          feather_radius_y);
GIMP_DEPRECATED_FOR(gimp_vectors_import_from_file)
gboolean gimp_path_import            (gint32           image_ID,
                                      const gchar     *filename,
                                      gboolean         merge,
                                      gboolean         scale);


G_END_DECLS

#endif /* __GIMP_PATHS_PDB_H__ */

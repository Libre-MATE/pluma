/*
 * pluma-spell-utils.h
 * This file is part of pluma
 *
 * Copyright (C) 2010 - Jesse van den Kieboom
 * Copyright (C) 2012-2021 MATE Developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef __PLUMA_SPELL_UTILS_H__
#define __PLUMA_SPELL_UTILS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

gboolean pluma_spell_utils_is_digit(const char *text, gssize length);

gboolean pluma_spell_utils_skip_no_spell_check(GtkTextIter *start,
                                               GtkTextIter *end);

G_END_DECLS

#endif /* __PLUMA_SPELL_UTILS_H__ */

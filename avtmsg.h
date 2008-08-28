/* 
 * avtmsg - message output for avatarsay
 * Copyright (c) 2007, 2008 Andreas K. Foerster <info@akfoerster.de>
 *
 * This file is part of AKFAvatar
 *
 * AKFAvatar is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AKFAvatar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* $Id: avtmsg.h,v 2.1 2008-08-28 18:16:23 akf Exp $ */

#ifndef AVTMSG_H
#define AVTMSG_H 1

#define PRGNAME "avatarsay"

/* 
 * "warning_msg", "notice_msg" and "error_msg" take 2 message strings
 * the second one may simply be NULL if you don't need it
 */

void warning_msg (const char *msg1, const char *msg2);
void notice_msg (const char *msg1, const char *msg2);
void error_msg (const char *msg1, const char *msg2);

#endif /* AVTMSG_H */

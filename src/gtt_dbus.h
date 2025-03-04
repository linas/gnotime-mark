/*
 * An interface to dbus for gnotime, that turns on/off the timer from dbus.
 *
 * Copyright (C) 2007 Michael Richardson <mcr@sandelman.ca>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#if WITH_DBUS

#ifndef GTT_DBUS_H
#define GTT_DBUS_H

/* Carry out all actions to setup the D-Bus and register signal handlers */
void gnotime_dbus_setup ();

#endif // GTT_DBUS_H

#endif // WITH_DBUS

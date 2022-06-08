#!/usr/bin/env python3

"""Rudimentary, temporay Python3 port of GnoTime

"""

################################################################################
#
# GnoTime - a to-do list organizer, diary and billing system
#
# Copyright (C) 1997,98 Eckehard Berns
# Copyright (C) 2001 Linas Vepstas <linas@linas.org>
# Copyright (C) 2010 Goedson Teixeira Paixao <goedson@debian.org>
# Copyright (C) 2022      Markus Prasser
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA
#
################################################################################

import gi
gi.require_version("Gtk", "3.0")

from gi.repository import Gtk

# Global constants
GTT_APP_NAME = "gnotime"
GTT_APP_PROPER_NAME = "GnoTime"
GTT_APP_TITLE = "Gnome Time Tracker"
VERSION = "2.99.0"

class Application(Gtk.Application):
    """The GnoTime application class

    """
    def __init__(self):
        super().__init__(application_id="com.github.goedson.gnotime")

class ApplicationWindow(Gtk.ApplicationWindow):
    """The main window of the Gtk application

    """
    def __init__(self):
        super().__init__()

        self.set_title(GTT_APP_TITLE + " " + VERSION)

        self.set_wmclass(GTT_APP_NAME, GTT_APP_PROPER_NAME)

        self.set_default_size(485, 272)

def create_app_window(caller):
    """Create the application window upon application activation

    """
    win = Gtk.ApplicationWindow()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    caller.add_window(win)

def main():
    """Script entry point of GnoTime

    """
    app = Application()
    app.connect("activate", create_app_window)
    app.run()

if __name__ == "__main__":
    main()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

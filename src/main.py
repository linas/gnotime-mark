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

from enum import Enum, unique # pylint: disable=wrong-import-order, wrong-import-position
from gi.repository import Gtk # pylint: disable=wrong-import-position

@unique
class BillRate(Enum):
    """Enumeration of different types of bill rates

    """
    REGULAR = 0
    OVERTIME = 1
    OVEROVER = 2
    FLAT_FEE = 3

@unique
class BillStatus(Enum):
    """Enumeration reflecting the billing status of an entity

    """
    HOLD = 0 # Needs review, will not appear in invoice
    BILL = 1 # Print on invoice
    PAID = 2 # Paid already, don't print on invoice anymore

@unique
class Billable(Enum):
    """Enumeration allowing to reflect if something is billable

    """
    BILLABLE = 1 # Billable time
    NOT_BILLABLE = 2 # Not billable to customer, internal only
    NO_CHARGE = 3 # Shows on invoice as "no charge/free"

@unique
class ProjectStatus(Enum):
    """Enumeration allowing to describe the project status

    """
    NO_STATUS = 0
    NOT_STARTED = 1
    IN_PROGRESS = 2
    ON_HOLD = 3
    CANCELLED = 4
    COMPLETED = 5

@unique
class Rank(Enum):
    """Enumeration allowing to define importances and urgencies

    """
    UNDEFINED = 0
    LOW = 1
    MEDIUM = 2
    HIGH = 3

# Global constants
GTT_APP_NAME = "gnotime"
GTT_APP_PROPER_NAME = "GnoTime"
GTT_APP_TITLE = "Gnome Time Tracker"
VERSION = "2.99.0"

# Global variables
config_show_tb_calendar = False
config_show_tb_ccp = False
config_show_tb_exit = True
config_show_tb_help = True
config_show_tb_journal = True
config_show_tb_new = True
config_show_tb_pref = False
config_show_tb_prop = True
config_show_tb_timer = True
config_show_tb_tips = True
config_show_toolbar = True

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

        widget = Toolbar()
        widget.show()

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        vbox.show()

        vbox.pack_start(widget, True, False, 0)

        self.add(vbox)

class Toolbar(Gtk.Toolbar):
    """Toolbar of the main application window

    """
    def __init__(self):
        super().__init__()

        position = 0

        if config_show_toolbar:
            if config_show_tb_new:
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        "document-new", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", new_project)
                btn.set_property("tooltip-text", "Create a New Project...")
                btn.show()
                self.insert(btn, position)
                position += 1
                separator = Gtk.SeparatorToolItem()
                separator.show()
                self.insert(separator, position)
                position += 1

def create_app_window(caller):
    """Create the application window upon application activation

    """
    win = ApplicationWindow()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    caller.add_window(win)

def grab_focus_cb(caller, wdgt):
    """Callback which makes given widget grab focus

    """
    wdgt.grab_focus()

def new_project(_caller):
    """Callback to create the new project dialog

    """
    title = Gtk.Entry() # TODO: Add mechanism for history
    desc = Gtk.Entry() # TODO: Add mechanism for history

    diag = Gtk.Dialog(title="New Project...")
    diag.add_button("OK", Gtk.ResponseType.OK)
    diag.add_button("Cancel", Gtk.ResponseType.CANCEL)
    diag.connect("response", project_name_desc)

    vbox = diag.get_content_area()

    title_label = Gtk.Label(label="Project Title")
    desc_label = Gtk.Label(label="Description")

    table = Gtk.Table(n_rows=2, n_columns=2, homogeneous=False)
    table.attach(
        title_label, 0, 1, 0, 1,
        Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL,
        Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL,
        2, 1
    )
    table.attach(
        title, 1, 2, 0, 1,
        Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL,
        Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL,
        2, 1
    )
    table.attach(
        desc_label, 0, 1, 1, 2,
        Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL,
        Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL,
        2, 1
    )
    table.attach(
        desc, 1, 2, 1, 2,
        Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL,
        Gtk.AttachOptions.EXPAND | Gtk.AttachOptions.FILL,
        2, 1
    )
    vbox.pack_start(table, False, False, 2)
    title_label.show()
    title.show()
    desc_label.show()
    desc.show()
    table.show()

    title.grab_focus()

    # Enter in the first entry forwards focus to the next one
    title.connect("activate", grab_focus_cb, desc)

    diag.run()

def project_name_desc(_caller, response):
    """Project creation callback

    """
    print("response: " + str(response))

def main():
    """Script entry point of GnoTime

    """
    app = Application()
    app.connect("activate", create_app_window)
    app.run()

if __name__ == "__main__":
    main()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

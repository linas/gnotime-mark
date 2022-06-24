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
ACTIVITY_REPORT = "activity.ghtml"
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

        # TODO: menus_create(GNOME_APP(app_window));

        widget = Toolbar()
        widget.show()

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        vbox.show()

        vbox.pack_start(widget, True, False, 0)

        status_vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        status_vbox.show()

        labels = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        labels.show()

        status_bar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        status_bar.show()
        separator = Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL)
        separator.show()
        status_vbox.pack_start(separator, False, False, 0)
        status_vbox.pack_start(labels, True, True, 0)
        status_bar.pack_start(status_vbox, True, True, 0)

        grip = Gtk.Statusbar()
        # TODO: grip.set_has_resize_grip(True)
        grip.show()
        status_bar.pack_start(grip, False, False, 0)

        status_day_time = Gtk.Label(label="00:00")
        status_day_time.show()
        labels.pack_start(status_day_time, False, True, 0)

        status_project = Gtk.Label(label="Timer is not running")
        status_project.show()
        labels.pack_start(status_project, False, True, 10)

        filler = Gtk.Label()
        filler.show()
        labels.pack_start(filler, True, True, 1)

        # TODO: Check this and all other STOCK_MEDIA
        status_timer = Gtk.Image.new_from_icon_name(
            icon_name="media-record", size=Gtk.IconSize.MENU
        )
        status_timer.show()
        status_bar.pack_end(status_timer, False, False, 1)

        # TODO: projects_tree = gtt_projects_tree_new();
        # g_signal_connect(projects_tree, "columns-setup-done",
        #     G_CALLBACK(projects_tree_columns_setup_done), NULL);
        # gtk_tree_view_set_reorderable(GTK_TREE_VIEW (projects_tree), TRUE);
        # g_signal_connect(projects_tree, "row-activated",
        #     G_CALLBACK(projects_tree_row_activated), NULL);

        global_na = NotesArea()
        global_na.show()
        vbox.pack_start(global_na, True, True, 0)

        vbox.pack_end(status_bar, False, False, 2)

        # TODO: global_na.add_projects_tree(projects_tree)

        vbox.show()
        self.add(vbox)

class NotesArea(Gtk.VPaned):
    """Class for displaying a bunch of widget related to project notes

    """
    def __init__(self): # pylint: disable=too-many-locals,too-many-statements
        super().__init__()

        ctree_holder = Gtk.ScrolledWindow()
        ctree_holder.show()
        self.add1(ctree_holder)

        leftright_pane = Gtk.HPaned()
        leftright_pane.show()

        vbox1 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        vbox1.show()

        hbox1 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        hbox1.show()

        label1 = Gtk.Label(label="Project Title:")
        label1.show()
        hbox1.pack_start(label1, False, False, 0)

        close_proj_button = Gtk.Button.new_from_icon_name(
            "window-close", Gtk.IconSize.MENU
        )
        close_proj_button.set_property(
            "tooltip-text", "Close the project subwindow"
        )
        close_proj_button.set_receives_default(True)
        close_proj_button.set_relief(Gtk.ReliefStyle.NONE)
        close_proj_button.show()
        hbox1.pack_start(close_proj_button, False, False, 0)

        proj_title_entry = Gtk.Entry()
        proj_title_entry.set_property(
            "tooltip-text", "Edit the project title in this box"
        )
        proj_title_entry.show()
        hbox1.pack_end(proj_title_entry, True, True, 4)

        vbox1.pack_start(hbox1, False, False, 0)

        hbox3 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        hbox3.show()

        label3 = Gtk.Label(label="Desc:")
        label3.show()
        hbox3.pack_start(label3, False, False, 0)

        proj_desc_entry = Gtk.Entry()
        proj_desc_entry.set_property(
            "tooltip-text", "Edit the project description"
        )
        proj_desc_entry.show()
        hbox3.pack_start(proj_desc_entry, True, True, 4)

        vbox1.pack_start(hbox3, False, False, 2)

        scrolledwindow1 = Gtk.ScrolledWindow()
        scrolledwindow1.set_shadow_type(Gtk.ShadowType.IN)
        scrolledwindow1.show()

        proj_notes_textview = Gtk.TextView()
        proj_notes_textview.show()
        scrolledwindow1.add(proj_notes_textview)

        vbox1.pack_start(scrolledwindow1, True, True, 2)

        leftright_pane.add1(vbox1)

        vbox2 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        vbox2.show()

        hbox2 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        hbox2.show()

        label2 = Gtk.Label(label="Diary Entry:")
        label2.show()
        hbox2.pack_start(label2, False, False, 0)

        diary_entry_combo = Gtk.ComboBox()
        diary_entry_combo.show()
        hbox2.pack_start(diary_entry_combo, True, True, 0)

        edit_diary_entry_button = Gtk.Button.new_from_stock(
            stock_id="gtk-edit"
        )
        # edit_diary_entry_button.set_property(
        #     "events",
            # GDK_POINTER_MOTION_MASK
            # | GDK_POINTER_MOTION_HINT_MASK
            # | GDK_BUTTON_PRESS_MASK
            # | GDK_BUTTON_RELEASE_MASK
        # )
        edit_diary_entry_button.set_receives_default(True)
        edit_diary_entry_button.show()
        hbox2.pack_start(edit_diary_entry_button, False, False, 0)

        new_diary_entry_button = Gtk.Button.new_from_icon_name(
            icon_name="document-new", size=Gtk.IconSize.BUTTON
        )
        new_diary_entry_button.set_property(
            "tooltip-text", "Create a new diary entry"
        )
        new_diary_entry_button.set_property(
            "use-underline", True
        )
        hbox2.pack_start(new_diary_entry_button, False, False, 0)

        close_diary_button = Gtk.Button.new_from_icon_name(
            icon_name="window-close", size=Gtk.IconSize.BUTTON
        )
        close_diary_button.set_relief(Gtk.ReliefStyle.NONE)
        close_diary_button.show()
        hbox2.pack_start(close_diary_button, False, False, 0)

        vbox2.pack_start(hbox2, False, True, 0)

        scrolledwindow2 = Gtk.ScrolledWindow()
        scrolledwindow2.set_shadow_type(Gtk.ShadowType.IN)
        scrolledwindow2.show()

        diary_notes_textview = Gtk.TextView()
        diary_notes_textview.show()
        scrolledwindow2.add(diary_notes_textview)

        vbox2.pack_start(scrolledwindow2, True, True, 0)

        leftright_pane.add2(vbox2)

        self.add2(leftright_pane)

class Project: # pylint: disable=too-few-public-methods,too-many-instance-attributes
    """Class representing a project

    """
    next_free_id = 1

    def __init__(self):
        self.title = ""
        self.desc = ""
        self.notes = ""
        self.cust_id = None

        self.min_interval = 3
        self.auto_merge_interval = 60
        self.auto_merge_gap = 60

        self.billrate = 10.0
        self.overtime_rate = 15.0
        self.overover_rate = 20.0
        self.flat_fee = 1000.0

        self.estimated_start = -1
        self.estimated_end = -1
        self.due_date = -1
        self.sizing = 0
        self.percent_complete = 0
        self.urgency = Rank.UNDEFINED
        self.importance = Rank.UNDEFINED
        self.status = ProjectStatus.NOT_STARTED

        self.task_list = None
        self.sub_projects = None
        self.parent = None
        self.listeners = None
        self.private_data = None

        self.being_destroyed = False
        self.frozen = False
        self.dirty_time = False

        self.secs_ever = 0
        self.secs_day = 0
        self.secs_yesterday = 0
        self.secs_week = 0
        self.secs_lastweek = 0
        self.secs_month = 0
        self.secs_year = 0

        self.proj_id = Project.next_free_id
        Project.next_free_id += 1

        # TODO: qof_instance_init (&self.inst, GTT_PROJECT_ID, global_book);

    @staticmethod
    def new_title_desc(title=None, description=None):
        """Create a new Project instance with given name and description

        """
        proj = Project()

        if title:
            proj.title = title
        if description:
            proj.desc = description

        return proj

class Toolbar(Gtk.Toolbar):
    """Toolbar of the main application window

    """
    def __init__(self): # pylint: disable=too-many-branches,too-many-statements
        super().__init__()

        self.spa = 0
        self.spb = 0
        self.spc = 0

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
                self.spa = position
                position += 1
            if config_show_tb_ccp:
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        "edit-cut", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", cut_project)
                btn.set_property("tooltip-text", "Cut Selected Project")
                self.insert(btn, position)
                position += 1
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        "edit-copy", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", copy_project)
                btn.set_property("tooltip-text", "Copy Selected Project")
                self.insert(btn, position)
                position += 1
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        "edit-paste", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", paste_project)
                btn.set_property("tooltip-text", "Paste Project")
                self.insert(btn, position)
                position += 1
                separator = Gtk.SeparatorToolItem()
                separator.show()
                self.insert(separator, position)
                self.spb = position
                if self.spa:
                    self.spb -= 1
                position += 1
            if config_show_tb_journal:
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        # TODO: Proper icon "Activity Journal"
                        "media-optical", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", show_report, ACTIVITY_REPORT)
                btn.set_property("tooltip-text", "View and Edit Timestamp Logs")
                self.insert(btn, position)
                position += 1
            if config_show_tb_prop:
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        "document-properties", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", menu_properties)
                btn.set_property("tooltip-text", "Edit Project Properties...")
                self.insert(btn, position)
                position += 1
            if config_show_tb_timer:
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        # TODO; Proper icon "Timer"
                        "media-record", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", menu_toggle_timer)
                btn.set_property("tooltip-text", "Start/Stop Timer")
                self.insert(btn, position)
                position += 1
            if config_show_tb_calendar:
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        # TODO; Proper icon "Calendar"
                        "document-print", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", edit_calendar)
                btn.set_property("tooltip-text", "View Calendar")
                self.insert(btn, position)
                position += 1
            if (config_show_tb_timer # pylint: disable=too-many-boolean-expressions
                    or config_show_tb_journal
                    or config_show_tb_calendar
                    or config_show_tb_prop) \
                    and (config_show_tb_pref
                         or config_show_tb_help
                         or config_show_tb_exit):
                separator = Gtk.SeparatorToolItem()
                separator.show()
                self.insert(separator, position)
                self.spc = position
                if self.spa:
                    self.spc -= 1
                if self.spb:
                    self.spc -= 1
                position += 1
            if config_show_tb_pref:
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        "preferences-system", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", menu_options)
                btn.set_property("tooltip-text", "Edit Preferences...")
                self.insert(btn, position)
                position += 1
            if config_show_tb_help:
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        "help-browser", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", help_popup)
                btn.set_property("tooltip-text", "User's Guide and Manual")
                self.insert(btn, position)
                position += 1
            if config_show_tb_exit:
                btn = Gtk.ToolButton(
                    icon_widget=Gtk.Image.new_from_icon_name(
                        "application-exit", Gtk.IconSize.LARGE_TOOLBAR
                    )
                )
                btn.connect("clicked", app_quit)
                self.insert(btn, position)
                position += 1

def app_quit(_caller):
    """TODO

    """

def create_app_window(caller):
    """Create the application window upon application activation

    """
    win = ApplicationWindow()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    caller.add_window(win)

def copy_project(_caller):
    """TODO

    """

def cut_project(_caller):
    """TODO

    """

def edit_calendar(_caller):
    """TODO

    """

def grab_focus_cb(caller, wdgt):
    """Callback which makes given widget grab focus

    """
    wdgt.grab_focus()

def help_popup(_caller):
    """TODO

    """

def menu_options(_caller):
    """TODO

    """

def menu_properties(_caller):
    """TODO

    """

def menu_toggle_timer(_caller):
    """TODO

    """

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

def paste_project(_caller):
    """TODO

    """

def project_name_desc(_caller, response):
    """Project creation callback

    """
    print("response: " + str(response))

def show_report(_caller, _report_file_name):
    """TODO

    """

def main():
    """Script entry point of GnoTime

    """
    app = Application()
    app.connect("activate", create_app_window)
    app.run()

if __name__ == "__main__":
    main()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

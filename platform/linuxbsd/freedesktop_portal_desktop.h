/**************************************************************************/
/*  freedesktop_portal_desktop.h                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             REDOT ENGINE                               */
/*                        https://redotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2024-present Redot Engine contributors                   */
/*                                          (see REDOT_AUTHORS.md)        */
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#ifdef DBUS_ENABLED

#include "core/os/thread.h"
#include "core/os/thread_safe.h"
#include "servers/display_server.h"

struct DBusMessage;
struct DBusConnection;
struct DBusMessageIter;

class FreeDesktopPortalDesktop : public Object {
private:
	bool unsupported = false;

	enum ReadVariantType {
		VAR_TYPE_UINT32, // u
		VAR_TYPE_BOOL, // b
		VAR_TYPE_COLOR, // (ddd)
	};

	static bool try_parse_variant(DBusMessage *p_reply_message, ReadVariantType p_type, void *r_value);
	// Read a setting from org.freekdesktop.portal.Settings
	bool read_setting(const char *p_namespace, const char *p_key, ReadVariantType p_type, void *r_value);

	static void append_dbus_string(DBusMessageIter *p_iter, const String &p_string);
	static void append_dbus_dict_options(DBusMessageIter *p_iter, const TypedArray<Dictionary> &p_options, HashMap<String, String> &r_ids);
	static void append_dbus_dict_filters(DBusMessageIter *p_iter, const Vector<String> &p_filter_names, const Vector<String> &p_filter_exts, const Vector<String> &p_filter_mimes);
	static void append_dbus_dict_string(DBusMessageIter *p_iter, const String &p_key, const String &p_value, bool p_as_byte_array = false);
	static void append_dbus_dict_bool(DBusMessageIter *p_iter, const String &p_key, bool p_value);
	static bool file_chooser_parse_response(DBusMessageIter *p_iter, const Vector<String> &p_names, const HashMap<String, String> &p_ids, bool &r_cancel, Vector<String> &r_urls, int &r_index, Dictionary &r_options);
	static bool color_picker_parse_response(DBusMessageIter *p_iter, bool &r_cancel, Color &r_color);

	struct ColorPickerData {
		Callable callback;
		String filter;
		String path;
	};

	struct ColorPickerCallback {
		Callable callback;
		Variant status;
		Variant color;
	};
	List<ColorPickerCallback> pending_color_cbs;

	Mutex color_picker_mutex;
	Vector<ColorPickerData> color_pickers;

	struct FileDialogData {
		Vector<String> filter_names;
		HashMap<String, String> option_ids;
		DisplayServer::WindowID prev_focus = DisplayServer::INVALID_WINDOW_ID;
		Callable callback;
		String filter;
		String path;
		bool opt_in_cb = false;
	};

	struct FileDialogCallback {
		Callable callback;
		Variant status;
		Variant files;
		Variant index;
		Variant options;
		bool opt_in_cb = false;
	};
	List<FileDialogCallback> pending_file_cbs;

	Mutex file_dialog_mutex;
	Vector<FileDialogData> file_dialogs;
	Thread monitor_thread;
	SafeFlag monitor_thread_abort;
	DBusConnection *monitor_connection = nullptr;

	String theme_path;
	Callable system_theme_changed;
	void _system_theme_changed_callback();
	bool _is_interface_supported(const char *p_iface);

	static void _thread_monitor(void *p_ud);

public:
	FreeDesktopPortalDesktop();
	~FreeDesktopPortalDesktop();

	bool is_supported() { return !unsupported; }
	bool is_file_chooser_supported();
	bool is_settings_supported();
	bool is_screenshot_supported();

	// org.freedesktop.portal.FileChooser methods.
	Error file_dialog_show(DisplayServer::WindowID p_window_id, const String &p_xid, const String &p_title, const String &p_current_directory, const String &p_root, const String &p_filename, DisplayServer::FileDialogMode p_mode, const Vector<String> &p_filters, const TypedArray<Dictionary> &p_options, const Callable &p_callback, bool p_options_in_cb);
	void process_callbacks();

	// org.freedesktop.portal.Settings methods.

	// Retrieve the system's preferred color scheme.
	// 0: No preference or unknown.
	// 1: Prefer dark appearance.
	// 2: Prefer light appearance.
	uint32_t get_appearance_color_scheme();
	Color get_appearance_accent_color();
	void set_system_theme_change_callback(const Callable &p_system_theme_changed) {
		system_theme_changed = p_system_theme_changed;
	}

	// Retrieve high-contrast setting.
	// -1: Unknown.
	// 0: Disabled.
	// 1: Enabled.
	uint32_t get_high_contrast();

	// org.freedesktop.portal.Screenshot methods.
	bool color_picker(const String &p_xid, const Callable &p_callback);
};

#endif // DBUS_ENABLED

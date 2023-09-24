/*************************************************************************/
/*  project_manager.cpp                                                  */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "version.h"
#include "project_manager.h"
#include "os/os.h"
#include "os/dir_access.h"
#include "os/file_access.h"
#include "editor_settings.h"
#include "scene/gui/separator.h"
#include "scene/gui/tool_button.h"
#include "io/config_file.h"

#include "scene/gui/line_edit.h"
#include "scene/gui/panel_container.h"

#include "scene/gui/empty_control.h"
#include "scene/gui/texture_frame.h"
#include "scene/gui/margin_container.h"
#include "io/resource_saver.h"

#include "editor_icons.h"



class NewProjectDialog : public ConfirmationDialog {

	OBJ_TYPE(NewProjectDialog,ConfirmationDialog);


	bool import_mode;
	Label *pp,*pn;
	Label *error;
	LineEdit *project_path;
	LineEdit *project_name;
	FileDialog *fdialog;

	bool _test_path() {

		error->set_text("");
		get_ok()->set_disabled(true);
		DirAccess *d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (d->change_dir(project_path->get_text())!=OK) {
			error->set_text("Invalid Path for Project, Path Must Exist!");
			memdelete(d);
			return false;
		}

		if (!import_mode) {

			if (d->file_exists("engine.cfg")) {

				error->set_text("Invalid Project Path (engine.cfg must not exist).");
				memdelete(d);
				return false;
			}

		} else {

			if (!d->file_exists("engine.cfg")) {

				error->set_text("Invalid Project Path (engine.cfg must exist).");
				memdelete(d);
				return false;
			}
		}

		memdelete(d);
		get_ok()->set_disabled(false);
		return true;

	}

	void _path_text_changed(const String& p_path) {

		_test_path();
		if (import_mode) {

			String sp=p_path;

			sp=sp.replace("\\","/");
			int lidx=sp.find_last("/");
			if (lidx!=-1) {
				sp=sp.substr(lidx+1,sp.length());
			}
			if (sp=="")
				sp="Imported Project";

			project_name->set_text(sp);
		}

	}

	void _file_selected(const String& p_path) {

		String p = p_path;
		if (import_mode) {
			if (p.ends_with("engine.cfg")) {

				p=p.get_base_dir();
			}
		}
		String sp = p.simplify_path();
		project_path->set_text(sp);
		_path_text_changed(p);
	}

	void _path_selected(const String& p_path) {

		String p = p_path;
		String sp = p.simplify_path();
		project_path->set_text(sp);
		_path_text_changed(p);

	}

	void _browse_path() {

		if (import_mode) {

			fdialog->set_mode(FileDialog::MODE_OPEN_FILE);
			fdialog->clear_filters();
			fdialog->add_filter("engine.cfg ; " _MKSTR(VERSION_NAME) " Project");
		} else {
			fdialog->set_mode(FileDialog::MODE_OPEN_DIR);
		}
		fdialog->popup_centered_ratio();
	}

	void _text_changed(const String& p_text) {
		_test_path();
	}

	void ok_pressed() {

		if (!_test_path())
			return;

		String dir;

		if (import_mode) {
			dir=project_path->get_text();


		} else {
			DirAccess *d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

			if (d->change_dir(project_path->get_text())!=OK) {
				error->set_text("Invalid Path for Project (changed anything?)");
				memdelete(d);
				return;
			}

			dir=d->get_current_dir();
			memdelete(d);

			FileAccess *f = FileAccess::open(dir.plus_file("/engine.cfg"),FileAccess::WRITE);
			if (!f) {
				error->set_text("Couldn't create engine.cfg in project path");
			} else {

				f->store_line("; Engine configuration file.");
				f->store_line("; It's best to edit using the editor UI, not directly,");
				f->store_line("; becausethe parameters that go here are not obvious.");
				f->store_line("; ");
				f->store_line("; Format: ");
				f->store_line(";   [section] ; section goes between []");
				f->store_line(";   param=value ; assign values to parameters");
				f->store_line("\n");
				f->store_line("[application]");
				f->store_line("name=\""+project_name->get_text()+"\"");
				f->store_line("icon=\"icon.png\"");

				memdelete(f);

				ResourceSaver::save(dir.plus_file("/icon.png"),get_icon("DefaultProjectIcon","EditorIcons"));
			}



		}

		dir=dir.replace("\\","/");
		if (dir.ends_with("/"))
			dir=dir.substr(0,dir.length()-1);
		String proj=dir.replace("/","::");
		EditorSettings::get_singleton()->set("projects/"+proj,dir);
		EditorSettings::get_singleton()->save();



		hide();
		emit_signal("project_created");

	}

protected:

	static void _bind_methods() {

		ObjectTypeDB::bind_method("_browse_path",&NewProjectDialog::_browse_path);
		ObjectTypeDB::bind_method("_text_changed",&NewProjectDialog::_text_changed);
		ObjectTypeDB::bind_method("_path_text_changed",&NewProjectDialog::_path_text_changed);
		ObjectTypeDB::bind_method("_path_selected",&NewProjectDialog::_path_selected);
		ObjectTypeDB::bind_method("_file_selected",&NewProjectDialog::_file_selected);
		ADD_SIGNAL( MethodInfo("project_created") );
	}

public:


	void set_import_mode(bool p_import ) {

		import_mode=p_import;
	}

	void show_dialog() {


		project_path->clear();
		project_name->clear();

		if (import_mode) {
			set_title("Import Existing Project:");
			pp->set_text("Project Path: (Must exist)");
			pn->set_text("Project Name:");
			pn->hide();
			project_name->hide();

			popup_centered(Size2(500,125));

		} else {
			set_title("Create New Project:");
			pp->set_text("Project Path:");
			pn->set_text("Project Name:");
			pn->show();
			project_name->show();

			popup_centered(Size2(500,145));

		}


		_test_path();
	}

	NewProjectDialog() {


		VBoxContainer *vb = memnew( VBoxContainer );
		add_child(vb);
		set_child_rect(vb);

		Label* l = memnew(Label);
		l->set_text("Project Path:");
		vb->add_child(l);
		pp=l;

		project_path = memnew( LineEdit );
		MarginContainer *mc = memnew( MarginContainer );
		vb->add_child(mc);
		HBoxContainer *pphb = memnew( HBoxContainer );
		mc->add_child(pphb);
		pphb->add_child(project_path);
		project_path->set_h_size_flags(SIZE_EXPAND_FILL);

		Button* browse = memnew( Button );
		pphb->add_child(browse);
		browse->set_text("Browse");
		browse->connect("pressed", this,"_browse_path");

		l = memnew(Label);
		l->set_text("Project Name:");
		l->set_pos(Point2(5,50));
		vb->add_child(l);
		pn=l;

		project_name = memnew( LineEdit );
		mc = memnew( MarginContainer );
		vb->add_child(mc);
		mc->add_child(project_name);
		project_name->set_text("New Game Project");


		l = memnew(Label);
		l->set_text("That's a BINGO!");
		vb->add_child(l);
		error=l;
		l->add_color_override("font_color",Color(1,0.4,0.3,0.8));
		l->set_align(Label::ALIGN_CENTER);

		get_ok()->set_text("Create");
		DirAccess *d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		project_path->set_text(d->get_current_dir());
		memdelete(d);

		fdialog = memnew( FileDialog );
		add_child(fdialog);
		fdialog->set_access(FileDialog::ACCESS_FILESYSTEM);
		project_name->connect("text_changed", this,"_text_changed");
		project_path->connect("text_changed", this,"_path_text_changed");
		fdialog->connect("dir_selected", this,"_path_selected");
		fdialog->connect("file_selected", this,"_file_selected");
		set_hide_on_ok(false);
		import_mode=false;
	}


};





void ProjectManager::_panel_draw(Node *p_hb) {

	HBoxContainer *hb = p_hb->cast_to<HBoxContainer>();

	hb->draw_line(Point2(0,hb->get_size().y+1),Point2(hb->get_size().x-10,hb->get_size().y+1),get_color("guide_color","Tree"));

	if (hb->get_meta("name")==selected) {

		hb->draw_style_box(get_stylebox("selected","Tree"),Rect2(Point2(),hb->get_size()-Size2(10,0)));
	}
}

void ProjectManager::_panel_input(const InputEvent& p_ev,Node *p_hb) {

	if (p_ev.type==InputEvent::MOUSE_BUTTON && p_ev.mouse_button.pressed && p_ev.mouse_button.button_index==BUTTON_LEFT) {

		selected = p_hb->get_meta("name");
		selected_main = p_hb->get_meta("main_scene");
		for(int i=0;i<scroll_childs->get_child_count();i++) {
			scroll_childs->get_child(i)->cast_to<CanvasItem>()->update();
		}
		erase_btn->set_disabled(false);
		open_btn->set_disabled(false);
		run_btn->set_disabled(selected_main=="");

		if (p_ev.mouse_button.doubleclick)
			_open_project(); //open if doubleclicked
	}
}

void ProjectManager::_load_recent_projects() {

	while(scroll_childs->get_child_count()>0) {
		memdelete( scroll_childs->get_child(0));
	}

	List<PropertyInfo> properties;
	EditorSettings::get_singleton()->get_property_list(&properties);

	Color font_color = get_color("font_color","Tree");

	for(List<PropertyInfo>::Element *E=properties.front();E;E=E->next()) {

		String _name = E->get().name;
		if (!_name.begins_with("projects/"))
			continue;

		String project = _name.get_slice("/",1);
		String path = EditorSettings::get_singleton()->get(_name);
		String conf=path.plus_file("engine.cfg");

		Ref<ConfigFile> cf = memnew( ConfigFile );
		Error err = cf->load(conf);
		ERR_CONTINUE(err!=OK);

		Ref<Texture> icon;
		String project_name="Unnamed Project";


		if (cf->has_section_key("application","icon")) {
			String appicon = cf->get_value("application","icon");
			if (appicon!="") {
				Image img;
				Error err = img.load(appicon.replace_first("res://",path+"/"));
				if (err==OK) {

					img.resize(64,64);
					Ref<ImageTexture> it = memnew( ImageTexture );
					it->create_from_image(img);
					icon=it;
				}
			}
		}

		if (cf->has_section_key("application","name")) {
			project_name = cf->get_value("application","name");
		}

		if (icon.is_null()) {
			icon=get_icon("DefaultProjectIcon","EditorIcons");
		}

		String main_scene;
		if (cf->has_section_key("application","main_scene")) {
			main_scene = cf->get_value("application","main_scene");
		}


		HBoxContainer *hb = memnew( HBoxContainer );
		hb->set_meta("name",project);
		hb->set_meta("main_scene",main_scene);
		hb->connect("draw",this,"_panel_draw",varray(hb));
		hb->connect("input_event",this,"_panel_input",varray(hb));
		TextureFrame *tf = memnew( TextureFrame );
		tf->set_texture(icon);
		hb->add_child(tf);
		VBoxContainer *vb = memnew(VBoxContainer);
		hb->add_child(vb);
		EmptyControl *ec = memnew( EmptyControl );
		ec->set_minsize(Size2(0,1));
		vb->add_child(ec);
		Label *title = memnew( Label(project_name) );
		title->add_font_override("font",get_font("large","Fonts"));
		title->add_color_override("font_color",font_color);
		vb->add_child(title);
		Label *fpath = memnew( Label(path) );
		vb->add_child(fpath);
		fpath->set_opacity(0.5);
		fpath->add_color_override("font_color",font_color);

		scroll_childs->add_child(hb);
	}

	erase_btn->set_disabled(selected=="");
	open_btn->set_disabled(selected=="");
	if (selected=="")
		run_btn->set_disabled(true);
}

void ProjectManager::_open_project() {


	if (selected=="") {
		return;
	}

	String path = EditorSettings::get_singleton()->get("projects/"+selected);
	print_line("OPENING: "+path+" ("+selected+")");

	List<String> args;


	args.push_back("-path");
	args.push_back(path);

	args.push_back("-editor");

	if (selected_main!="") {
		args.push_back(selected_main);
	}

	String exec = OS::get_singleton()->get_executable_path();

	OS::ProcessID pid=0;
	Error err = OS::get_singleton()->execute(exec,args,false,&pid);
	ERR_FAIL_COND(err);

	get_scene()->quit();

}

void ProjectManager::_run_project() {


	if (selected=="") {
		return;
	}

	String path = EditorSettings::get_singleton()->get("projects/"+selected);
	print_line("OPENING: "+path+" ("+selected+")");

	List<String> args;


	args.push_back("-path");
	args.push_back(path);

	String exec = OS::get_singleton()->get_executable_path();

	OS::ProcessID pid=0;
	Error err = OS::get_singleton()->execute(exec,args,false,&pid);
	ERR_FAIL_COND(err);

//	get_scene()->quit(); do not quit

}

void ProjectManager::_scan_dir(DirAccess *da,float pos, float total,List<String> *r_projects) {


	List<String> subdirs;
	da->list_dir_begin();
	String n = da->get_next();
	while(n!=String()) {
		if (da->current_is_dir() && !n.begins_with(".")) {
			subdirs.push_front(n);
		} else if (n=="engine.cfg") {
			r_projects->push_back(da->get_current_dir());
		}
		n=da->get_next();
	}
	da->list_dir_end();
	int m=0;
	for(List<String>::Element *E=subdirs.front();E;E=E->next()) {

		da->change_dir(E->get());

		float slice=total/subdirs.size();
		_scan_dir(da,pos+slice*m,slice,r_projects);
		da->change_dir("..");
		m++;
	}


}


void ProjectManager::_scan_begin(const String& p_base) {

	print_line("SCAN PROJECTS AT: "+p_base);
	List<String> projects;
	DirAccess *da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	da->change_dir(p_base);
	_scan_dir(da,0,1,&projects);
	memdelete(da);
	print_line("found: "+itos(projects.size())+" projects.");

	for(List<String>::Element *E=projects.front();E;E=E->next()) {
		String proj=E->get().replace("/","::");
		EditorSettings::get_singleton()->set("projects/"+proj,E->get());

	}
	EditorSettings::get_singleton()->save();
	_load_recent_projects();

}

void ProjectManager::_scan_projects() {

	scan_dir->popup_centered_ratio();

}


void ProjectManager::_new_project()  {

	npdialog->set_import_mode(false);
	npdialog->show_dialog();
}


void ProjectManager::_import_project()  {

	npdialog->set_import_mode(true);
	npdialog->show_dialog();
}

void ProjectManager::_erase_project_confirm()  {

	if (selected=="") {
		return;
	}

	EditorSettings::get_singleton()->erase("projects/"+selected);
	EditorSettings::get_singleton()->save();
	selected="";
	selected_main="";
	_load_recent_projects();

}

void ProjectManager::_erase_project()  {

	if (selected=="")
		return;


	erase_ask->set_text("Erase project from list?? (Folder contents will not be modified)");
	erase_ask->popup_centered(Size2(300,100));

}


void ProjectManager::_exit_dialog()  {

	get_scene()->quit();
}

void ProjectManager::_bind_methods() {

	ObjectTypeDB::bind_method("_open_project",&ProjectManager::_open_project);
	ObjectTypeDB::bind_method("_run_project",&ProjectManager::_run_project);
	ObjectTypeDB::bind_method("_scan_projects",&ProjectManager::_scan_projects);
	ObjectTypeDB::bind_method("_scan_begin",&ProjectManager::_scan_begin);
	ObjectTypeDB::bind_method("_import_project",&ProjectManager::_import_project);
	ObjectTypeDB::bind_method("_new_project",&ProjectManager::_new_project);
	ObjectTypeDB::bind_method("_erase_project",&ProjectManager::_erase_project);
	ObjectTypeDB::bind_method("_erase_project_confirm",&ProjectManager::_erase_project_confirm);
	ObjectTypeDB::bind_method("_exit_dialog",&ProjectManager::_exit_dialog);
	ObjectTypeDB::bind_method("_load_recent_projects",&ProjectManager::_load_recent_projects);
	ObjectTypeDB::bind_method("_panel_draw",&ProjectManager::_panel_draw);
	ObjectTypeDB::bind_method("_panel_input",&ProjectManager::_panel_input);

}

ProjectManager::ProjectManager() {

	int margin = get_constant("margin","Dialogs");
	int button_margin = get_constant("button_margin","Dialogs");

	// load settings
	if (!EditorSettings::get_singleton())
		EditorSettings::create();


	set_area_as_parent_rect();
	Panel *panel = memnew( Panel );
	add_child(panel);
	panel->set_area_as_parent_rect();

	VBoxContainer *vb = memnew( VBoxContainer );
	panel->add_child(vb);
	vb->set_area_as_parent_rect(20);


	Label *l = memnew( Label );
	l->set_text(_MKSTR(VERSION_NAME)" - Project Manager");
	l->add_font_override("font",get_font("large","Fonts"));
	l->set_align(Label::ALIGN_CENTER);
	vb->add_child(l);
	l = memnew( Label );
	l->set_text("v"VERSION_MKSTRING);
	//l->add_font_override("font",get_font("bold","Fonts"));
	l->set_align(Label::ALIGN_CENTER);
	vb->add_child(l);
	vb->add_child(memnew(HSeparator));
	vb->add_margin_child("\n",memnew(Control));


	HBoxContainer *tree_hb = memnew( HBoxContainer);
	vb->add_margin_child("Recent Projects:",tree_hb,true);

	PanelContainer *pc = memnew( PanelContainer);
	pc->add_style_override("panel",get_stylebox("bg","Tree"));
	tree_hb->add_child(pc);
	pc->set_h_size_flags(SIZE_EXPAND_FILL);

	scroll = memnew( ScrollContainer );
	pc->add_child(scroll);
	scroll->set_enable_h_scroll(false);

	VBoxContainer *tree_vb = memnew( VBoxContainer);
	tree_hb->add_child(tree_vb);	
	scroll_childs = memnew( VBoxContainer );
	scroll_childs->set_h_size_flags(SIZE_EXPAND_FILL);
	scroll->add_child(scroll_childs);

	//HBoxContainer *hb = memnew( HBoxContainer );
	//vb->add_child(hb);

	Button *open = memnew( Button );
	open->set_text("Edit");
	tree_vb->add_child(open);
	open->connect("pressed", this,"_open_project");
	open_btn=open;

	Button *run = memnew( Button );
	run->set_text("Run");
	tree_vb->add_child(run);
	run->connect("pressed", this,"_run_project");
	run_btn=run;

	tree_vb->add_child(memnew( HSeparator ));

	Button *scan = memnew( Button );
	scan->set_text("Scan");
	tree_vb->add_child(scan);
	scan->connect("pressed", this,"_scan_projects");

	tree_vb->add_child(memnew( HSeparator ));

	scan_dir = memnew( FileDialog );
	scan_dir->set_access(FileDialog::ACCESS_FILESYSTEM);
	scan_dir->set_mode(FileDialog::MODE_OPEN_DIR);
	add_child(scan_dir);
	scan_dir->connect("dir_selected",this,"_scan_begin");


	Button* create = memnew( Button );
	create->set_text("New Project");
	tree_vb->add_child(create);
	create->connect("pressed", this,"_new_project");

	Button* import = memnew( Button );
	import->set_text("Import");
	tree_vb->add_child(import);
	import->connect("pressed", this,"_import_project");


	Button* erase = memnew( Button );
	erase->set_text("Erase");
	tree_vb->add_child(erase);
	erase->connect("pressed", this,"_erase_project");
	erase_btn=erase;


	tree_vb->add_spacer();

	Button * cancel = memnew( Button );
	cancel->set_text("Exit");
	tree_vb->add_child(cancel);
	cancel->connect("pressed", this,"_exit_dialog");


	vb->add_margin_child("\n",memnew(Control));
	vb->add_child(memnew(HSeparator));

	l = memnew( Label );
	String cp;
	cp.push_back(0xA9);
	cp.push_back(0);
	l->set_text(cp+" 2008-2012 Juan Linietsky, Ariel Manzur.");
	l->set_align(Label::ALIGN_CENTER);
	vb->add_child(l);


	erase_ask = memnew( ConfirmationDialog );
	erase_ask->get_ok()->set_text("Erase");
	erase_ask->get_ok()->connect("pressed", this,"_erase_project_confirm");

	add_child(erase_ask);

	OS::get_singleton()->set_low_processor_usage_mode(true);

	npdialog = memnew( NewProjectDialog );
	add_child(npdialog);

	Ref<Theme> theme = memnew( Theme );
	editor_register_icons(theme);
	set_theme(theme);

	npdialog->connect("project_created", this,"_load_recent_projects");
	_load_recent_projects();
	//get_ok()->set_text("Open");
	//get_ok()->set_text("Exit");

}


ProjectManager::~ProjectManager() {

	if (EditorSettings::get_singleton())
		EditorSettings::destroy();
}

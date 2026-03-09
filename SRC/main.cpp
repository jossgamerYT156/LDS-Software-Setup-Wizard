#include <cstddef>
// Specific GTKMM Include. Required.
#include <gtkmm.h>
// End of GTKMM Specific Includes.
#include <algorithm> // For future use.
#include <filesystem>
#include <fstream>
#include <glibmm/dispatcher.h>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <setupWizard.hpp>

// We kinda need this one to write files and create directories.
namespace fs = std::filesystem;

// Structure to hold our optional feature data
struct OptionalFeature {
  std::string name;        // OptIn Feature name, obviously.
  std::string source_path; // Relative to OPTIN/
  std::string category;    // Software, Service, or Docs
  bool selected = true;    // Very obvious.
};

// Global value to check if the user cares about overwritting stuff.
bool userDoesNotCare = false;

// Define our main window method.
class RetroInstaller : public Gtk::Window {
private:
  Gtk::MessageDialog *installDialog = nullptr;
  Glib::Dispatcher dispatcher;
  std::thread *workerThread;

public:
  RetroInstaller()
      // Fixed weird nullptr issue from Woynert@NixOS - "NullPointer Exception
      // after Installation Finishes".
      : current_page(0), box_main(Gtk::ORIENTATION_VERTICAL, 0),
        box_bottom(Gtk::ORIENTATION_HORIZONTAL, 10),
        box_buttons(Gtk::ORIENTATION_HORIZONTAL, 5) {
    set_title(ProgramWindowName); // This is the title you see when you start
                                  // the application's setup.
    set_default_size(550, 420);   // You may change this if strictly necessary.
    set_resizable(false);
    set_position(Gtk::WIN_POS_CENTER);

    load_config();
    load_styles(); // Inject Windows 95 CSS

    // Check if the config contains the Opt-In Features flag set.
    if (config["HASOPTINFEATURES"] == "True") {
      load_features();
    }

    build_ui();

    add(box_main);
    add_events(Gdk::KEY_PRESS_MASK);

    show_all_children();
    update_page_visibility();
  }

protected:
  // This is what controls the F2 About Window, we never touch this... well,
  // almost never.
  bool on_key_press_event(GdkEventKey *key_event) override {
    if (key_event->keyval == GDK_KEY_F2) {
      show_about_dialog();
      return true;
    }
    return Gtk::Window::on_key_press_event(key_event);
  }

private:
  std::map<std::string, std::string> config;
  std::vector<OptionalFeature> features;
  int current_page;

  // Boxes definition.
  Gtk::Box box_main;
  Gtk::Notebook notebook;
  Gtk::Box box_bottom;
  Gtk::Box box_buttons;
  Gtk::Separator separator;

  // Buttons text and ids
  Gtk::Button btn_back{UIButton_Back};
  Gtk::Button btn_next{UIButton_Next};
  Gtk::Button btn_finish{UIButton_Finalize};
  Gtk::Button btn_cancel{UIButton_Cancel};

  struct ModelColumns : public Gtk::TreeModel::ColumnRecord {
    Gtk::TreeModelColumn<bool> col_enabled;
    Gtk::TreeModelColumn<std::string> col_name;
    Gtk::TreeModelColumn<std::string> col_type;
    ModelColumns() {
      add(col_enabled);
      add(col_name);
      add(col_type);
    }
  };
  ModelColumns feature_columns;
  Glib::RefPtr<Gtk::ListStore> feature_model;

  Gtk::FileChooserButton *btn_path_chooser;
  Gtk::CheckButton chk_accept{AcceptLicenseAgreement};
  Gtk::CheckButton *chk_open_app;
  Gtk::CheckButton *chk_read_me;

  // Resume View
  Gtk::TextView *resume_tv;

  void load_styles() {
    auto screen = Gdk::Screen::get_default();
    auto css_provider = Gtk::CssProvider::create();

    try {
      // Try to load the CSS, if the window manager allows it.
      css_provider->load_from_data(win95_css);
      Gtk::StyleContext::add_provider_for_screen(
          screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
    } catch (const Gtk::CssProviderError &ex) {
      std::cerr << "CSS Load Error: " << ex.what()
                << std::endl; // Else, report it.
    }
  }

  // Main configuration loading
  void load_config() {
    std::ifstream file("setup.conf");
    std::string line;
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      size_t sep = line.find('=');
      if (sep != std::string::npos) {
        std::string key = line.substr(0, sep);
        std::string val = line.substr(sep + 1);
        key.erase(key.find_last_not_of(" \t\n\r") + 1);
        val.erase(0, val.find_first_not_of(" \t\n\r"));
        size_t comment = val.find('#');
        if (comment != std::string::npos)
          val = val.substr(0, comment);
        val.erase(val.find_last_not_of(" \t\n\r") + 1);
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
          val = val.substr(1, val.size() - 2);
        config[key] = val;
      }
    }

    // Small check to ensure the installer's Global installation are not run by
    // non-root user, as this will fail regardless.
    if (config["DEFINSTTYPE"] == "Global" && getuid() != 0) {
      std::cerr << "Error: This installer must be run as root for global "
                   "installations."
                << std::endl;
      Gtk::MessageDialog d(
          "Global installation requires ROOT access, please rerun this "
          "installer with 'sudo' after reviewing the program's contents.",
          false, Gtk::MESSAGE_ERROR);
      d.run();
      exit(1);
    }
    // Default fallback values.
    if (config["PROGNAME"].empty())
      config["PROGNAME"] = "Unspecified Software";
    if (config["INSTDIRNAME"].empty())
      config["INSTDIRNAME"] = "UnsignedSoftware";
    if (config["DEFINSTDIR"].empty())
      config["DEFINSTDIR"] = "System/Programs";
    if (config["DEFINSTTYPE"].empty())
      config["DEFINSTTYPE"] = "User";
    if (config["HASCOPYRIGHT"].empty())
      config["HASCOPYRIGHT"] = "False";
    if (config["ASKIFOPENONEND"].empty())
      config["ASKIFOPENONEND"] = "True";
  }

  void load_features() {
    std::ifstream file(config["FEATURELISTFILE"]);
    if (!file.is_open())
      return;

    std::string line, current_cat;
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      if (line.back() == ':') {
        current_cat = line.substr(0, line.size() - 1);
        continue;
      }
      size_t dash = line.find("- ");
      size_t sep = line.find(": ");
      if (dash != std::string::npos && sep != std::string::npos) {
        OptionalFeature feat;
        feat.category = current_cat;
        feat.name = line.substr(dash + 2, sep - (dash + 2));
        feat.source_path = line.substr(sep + 2);
        features.push_back(feat);
      }
    }
  }

  uintmax_t calculate_dir_size(const fs::path &p) {
    if (!fs::exists(p))
      return 0;
    if (fs::is_regular_file(p))
      return fs::file_size(p);
    uintmax_t size = 0;
    try {
      for (const auto &entry : fs::recursive_directory_iterator(
               p, fs::directory_options::skip_permission_denied)) {
        if (fs::is_regular_file(entry))
          size += fs::file_size(entry);
      }
    } catch (...) {
      return 0;
    }
    return size;
  }

  void update_resume_text() {
    std::string resume;
    resume +=
        "You're Installing the following Software: " + config["PROGNAME"] +
        "\n";

    fs::path chosen_path = btn_path_chooser->get_filename();
    if (chosen_path.empty())
      chosen_path = fs::path(getenv("HOME")) / config["DEFINSTDIR"];
    fs::path final_dest = (config["MKDIR"] == "True")
                              ? chosen_path / config["INSTDIRNAME"]
                              : chosen_path;

    resume += "Installation Path: " + final_dest.string() + "\n";
    resume += "Installation Mode: " + config["DEFINSTTYPE"] + "\n\n";
    std::string meassure_unit = "MB";
    uintmax_t total_bytes = 0;
    if (fs::exists("Binary"))
      total_bytes += calculate_dir_size("Binary");
    if (config["HASREADME"] == "True" && fs::exists(config["READMEFILE"])) {
      total_bytes += fs::file_size(config["READMEFILE"]);
    }

    if (config["HASOPTINFEATURES"] == "True" && feature_model) {
      resume += "Selected Optional Features:\n";
      bool any_selected = false;
      int idx = 0;
      for (auto row : feature_model->children()) {
        if (row[feature_columns.col_enabled]) {
          resume += " - " + std::string(row[feature_columns.col_name]) + " (" +
                    std::string(row[feature_columns.col_type]) + ")\n";
          fs::path src =
              fs::path(config["OPTINDIR"]) / features[idx].source_path;
          total_bytes += calculate_dir_size(src);
          any_selected = true;
        }
        idx++;
      }
      if (!any_selected)
        resume += " - No 'Optional Features' will be installed.\n";
      resume += "\n";
    }
    double size;
    // double mb = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
    // Awful way to get KB/MB/GB meassures.
    double kb = total_bytes / 1024.0;
    double mb = kb / 1024.0;
    double gb = mb / 1024.0;
    if (gb >= 1) {
      size = gb;
      meassure_unit = "GB";
    } else if (mb >= 1) {
      size = mb;
      meassure_unit = "MB";
    } else {
      size = kb;
      meassure_unit = "KB";
    }
    char size_buf[64];
    snprintf(size_buf, sizeof(size_buf), "%.2f %s", size,
             meassure_unit.c_str());
    resume += "Total Space Required: " + std::string(size_buf);

    resume_tv->get_buffer()->set_text(resume);
  }

  void show_about_dialog() {
    // Hardcoded string for the "About" window, made specifically to avoid weird
    // formatting issues with GTKmm->Markdown
    Gtk::Dialog about_dialog(ProgramAboutWindowName, *this);
    // Else remains the same as of 0.0.22-23
    about_dialog.set_default_size(350, 200);
    auto content = about_dialog.get_content_area();
    content->set_spacing(15);
    content->set_margin_start(20);
    content->set_margin_end(20);
    content->set_margin_top(20);
    content->set_margin_bottom(20);

    auto h_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 15);
    if (fs::exists("Icons/SetupWizardIcon.png")) {
      h_box->pack_start(
          *Gtk::make_managed<Gtk::Image>("Icons/SetupWizardIcon.png"),
          Gtk::PACK_SHRINK);
    } else if (fs::exists("logo.svg")) {
      h_box->pack_start(*Gtk::make_managed<Gtk::Image>("logo.svg"),
                        Gtk::PACK_SHRINK);
    }

    auto v_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 5);
    auto lbl_name = Gtk::make_managed<Gtk::Label>();
    lbl_name->set_markup(
        "<span weight='bold' size='large'>Software Setup Wizard©</span>");
    lbl_name->set_xalign(0);
    auto lbl_cp = Gtk::make_managed<Gtk::Label>(
        "© " + config["COPYHOLDER"] +
        "\nAll rights reserved.\n\nPowered by LDS Softworks "
        "LLC©"); // You can use the installer as you please, but
                 // we kindly ask you to not remove this
                 // "Powered by" string, as this we use as
                 // "Attribution" rights, thanks :)
    lbl_cp->set_xalign(0);

    v_box->pack_start(*lbl_name, Gtk::PACK_SHRINK);
    v_box->pack_start(
        *Gtk::make_managed<Gtk::Label>(
            InstallerVersion // Macro defined at the start of the file, this is
                             // hardcoded on each build. from the header file.
            ),
        Gtk::PACK_SHRINK);
    v_box->pack_start(*lbl_cp, Gtk::PACK_SHRINK);
    h_box->pack_start(*v_box, Gtk::PACK_EXPAND_WIDGET);
    content->pack_start(*h_box, Gtk::PACK_EXPAND_WIDGET);

    about_dialog.add_button("OK", Gtk::RESPONSE_OK);
    about_dialog.show_all_children();
    about_dialog.run();
  }

  void update_dialog_box() {
    installDialog->set_message("Installation Complete!");
    installDialog->set_secondary_text(
        "You may now close this dialog and exit the installer.");
    // Commented unused value.
    installDialog->signal_response().connect([this](int /*response*/) {
      installDialog->hide();
      delete installDialog;
      installDialog = nullptr;
    });
    installDialog->set_deletable(true);
    installDialog->add_button("Close", Gtk::RESPONSE_CLOSE);
  }

  void build_ui() {
    notebook.set_show_tabs(false);
    notebook.set_show_border(false);
    dispatcher.connect(
        sigc::mem_fun(*this, &RetroInstaller::update_dialog_box));

    // --- PAGE 0: Welcome ---
    auto p0 = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 20);
    p0->set_border_width(20);
    auto img_sidebar = Gtk::make_managed<Gtk::Image>();
    if (fs::exists(config["ICON"]))
      img_sidebar->set(config["ICON"]);
    else if (fs::exists("Icons/setup.png"))
      img_sidebar->set("Icons/setup.png");
    p0->pack_start(*img_sidebar, Gtk::PACK_SHRINK);

    auto p0_text = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 10);
    auto lbl_title = Gtk::make_managed<Gtk::Label>();
    lbl_title->set_markup("<span size='x-large' weight='bold'>Welcome to the " +
                          config["PROGNAME"] + " Setup</span>");
    lbl_title->set_xalign(0);
    p0_text->pack_start(*lbl_title, Gtk::PACK_SHRINK);

    // Overcomplicated logic to determine which welcome message to show.
    // This is why i normally try to avoid C++, as this is just a nightmare to
    // read, but well, it works, so who am i to judge? :v
    std::string welcome_msg;
    // Check if the key exists in the map at all
    auto it = config.find("HASOPTINFEATURES");
    bool keyExists = (it != config.end());

    // Safely get the value (default to empty string if missing)
    std::string optInVal = keyExists ? it->second : "";
    // Check for the Feature List file
    bool featureFileExists = config.count("FEATURELISTFILE") &&
                             fs::exists(config["FEATURELISTFILE"]);
    // Determine the message
    if (keyExists && optInVal == "True" && featureFileExists) {
      welcome_msg =
          "This wizard will guide you through the installation of " +
          config["PROGNAME"] +
          " and its optional features in your PC."; // This is for if the
                                                    // features are available,
                                                    // and the file exists.
      // No, i will not add a check for seeing if the thing has data, is it
      // there or not, YOUR problem if it has data or not.
    } else {
      // This handles the "NULL" (missing), "False", or missing file cases,
      // because if it is missing, we cannot have a true or false value, so we
      // just default to the basic "Single program install" message.
      welcome_msg = "This wizard will guide you through the installation of " +
                    config["PROGNAME"] + " in your PC.";
    }
    auto lbl_desc = Gtk::make_managed<Gtk::Label>(welcome_msg);
    lbl_desc->set_xalign(0);
    lbl_desc->set_line_wrap(true);
    p0_text->pack_start(*lbl_desc, Gtk::PACK_SHRINK);
    p0->pack_start(*p0_text, Gtk::PACK_EXPAND_WIDGET);
    notebook.append_page(*p0);

    // --- PAGE 1: License ---
    auto p1 = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 10);
    p1->set_border_width(15);
    auto scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
    auto tv = Gtk::make_managed<Gtk::TextView>();
    if (!fs::exists(config["LICENSEFILE"]) || config["LICENSEFILE"].empty() ||
        !fs::exists("./LICENSE")) {
      tv->get_buffer()->set_text(ENOLICENSEADVISORY);
    } else {
      std::ifstream lic(config["LICENSEFILE"]);
      if (lic.is_open())
        tv->get_buffer()->set_text(
            std::string((std::istreambuf_iterator<char>(lic)),
                        std::istreambuf_iterator<char>()));
    }
    tv->set_editable(false);
    tv->set_wrap_mode(Gtk::WRAP_WORD);
    scroller->add(*tv);
    p1->pack_start(*scroller, Gtk::PACK_EXPAND_WIDGET);
    p1->pack_start(chk_accept, Gtk::PACK_SHRINK);
    chk_accept.signal_toggled().connect(
        [this] { btn_next.set_sensitive(chk_accept.get_active()); });
    notebook.append_page(*p1);

    // --- OPTIONAL PAGE: Features ---
    if (config["HASOPTINFEATURES"] == "True") {
      auto p2 = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 10);
      p2->set_border_width(15);
      p2->pack_start(
          *Gtk::make_managed<Gtk::Label>(
              "Select the optional features you wish to install:", 0),
          Gtk::PACK_SHRINK);

      auto feat_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
      auto feat_view = Gtk::make_managed<Gtk::TreeView>();
      feature_model = Gtk::ListStore::create(feature_columns);
      feat_view->set_model(feature_model);

      for (const auto &f : features) {
        auto row = *(feature_model->append());
        row[feature_columns.col_enabled] =
            false; // Default to not selected, user can select what they want.
        row[feature_columns.col_name] = f.name;
        row[feature_columns.col_type] = f.category;
      }

      feat_view->append_column_editable("Install?",
                                        feature_columns.col_enabled);
      feat_view->append_column("Feature Name", feature_columns.col_name);
      feat_view->append_column("Feature Category", feature_columns.col_type);

      feat_scroll->add(*feat_view);
      p2->pack_start(*feat_scroll, Gtk::PACK_EXPAND_WIDGET);
      notebook.append_page(*p2);
    }

    // --- PAGE: Path ---
    auto p3 = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 15);
    p3->set_border_width(20);
    p3->pack_start(
        *Gtk::make_managed<Gtk::Label>("Choose the installation directory:", 0),
        Gtk::PACK_SHRINK);
    btn_path_chooser = Gtk::make_managed<Gtk::FileChooserButton>(
        "Select Folder", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    fs::path base_path;
    if (config["DEFINSTDIR"] != "" || config["DEFINSTTYPE"] != "UserDir") {
      base_path =
          (config["DEFINSTTYPE"] == "Global")
              ? "/usr/bin"
              : fs::path(getenv("HOME")); // Default to $HOME if UserDir, or
                                          // empty, else, use the provided path.
    } else {
      base_path = (config["DEFINSTTYPE"] == "Global")
                      ? "/usr/bin"
                      : fs::path(getenv("HOME")) / config["DEFINSTDIR"];
    }
    btn_path_chooser->set_current_folder(base_path.string());
    p3->pack_start(*btn_path_chooser, Gtk::PACK_SHRINK);
    notebook.append_page(*p3);

    // --- PAGE: Pre-Installation Resume ---
    auto p4_resume = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 10);
    p4_resume->set_border_width(20);
    auto lbl_resume = Gtk::make_managed<Gtk::Label>("Before We Install:");
    lbl_resume->set_xalign(0);
    p4_resume->pack_start(*lbl_resume, Gtk::PACK_SHRINK);

    auto resume_scroller = Gtk::make_managed<Gtk::ScrolledWindow>();
    resume_tv = Gtk::make_managed<Gtk::TextView>();
    resume_tv->set_editable(false);
    resume_tv->set_wrap_mode(Gtk::WRAP_WORD);
    resume_scroller->add(*resume_tv);
    p4_resume->pack_start(*resume_scroller, Gtk::PACK_EXPAND_WIDGET);
    notebook.append_page(*p4_resume);

    // --- PAGE: Final ---
    auto p5 = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 10);
    p5->set_border_width(20);
    p5->pack_start(*Gtk::make_managed<Gtk::Label>("Installation Complete."),
                   Gtk::PACK_SHRINK);
    p5->pack_start(
        *Gtk::make_managed<Gtk::Label>(
            config["PROGNAME"] + "Has been installed on your PC.", 0),
        Gtk::PACK_SHRINK);
    p5->pack_start(*Gtk::make_managed<Gtk::Label>(
                       "Click Finish to exit the Setup Wizard."),
                   Gtk::PACK_SHRINK);
    chk_open_app =
        Gtk::make_managed<Gtk::CheckButton>("Launch " + config["PROGNAME"]);
    if (config["HASREADME"] == "True" && fs::exists(config["READMEFILE"])) {
      chk_read_me = Gtk::make_managed<Gtk::CheckButton>("Open Read Me");
    } else {
      // Fallback so the pointer isn't junk even if the file is missing
      // FIX: __NULLPTR_EXCEPTION__ from Woynert@NixOS Bug Report: "Crashes
      // after installation finishes with a Null-Pointer Exception"
      chk_read_me = Gtk::make_managed<Gtk::CheckButton>(
          "Open Read Me (Not Found)"); // Set the button's content regardless,
                                       // but modify its contents to ensure we
                                       // don't have NULL values.
      chk_read_me->set_sensitive(
          false); // Disable checking it, to avoid it being opened on end.
    }
    // This check disables the main-binary button if it is not found on the
    // Binary Directory, of course this makes no sense (that's why it is
    // commented), but it helps to avoid tampered-setups, you may enable this if
    // you wish to, but it is redundant, as the on_finish(); call has its own
    // tamper-check on files.
    // if (!fs::exists("Binary/" +
    // config["MAINBINNAME"])) {
    //   chk_open_app = Gtk::make_managed<Gtk::CheckButton>(
    //       "Launch " + config["PROGNAME"] + " (Main Binary Not Found)");
    //   chk_open_app->set_sensitive(false);
    // }

    p5->pack_start(*chk_open_app, Gtk::PACK_SHRINK);
    p5->pack_start(*chk_read_me,
                   Gtk::PACK_SHRINK); // This now always points to something,
                                      // even if no readme is available.
    notebook.append_page(*p5);

    // --- Bottom Layout ---
    box_main.pack_start(notebook, Gtk::PACK_EXPAND_WIDGET);
    box_main.pack_start(separator, Gtk::PACK_SHRINK);
    box_bottom.set_border_width(10);
    box_bottom.pack_start(*Gtk::make_managed<Gtk::Label>(ProgramWindowName),
                          Gtk::PACK_SHRINK);
    auto spacer = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL);
    box_bottom.pack_start(*spacer, Gtk::PACK_EXPAND_PADDING);
    box_bottom.pack_end(box_buttons, Gtk::PACK_SHRINK);
    box_buttons.pack_start(btn_back, Gtk::PACK_SHRINK);
    box_buttons.pack_start(btn_next, Gtk::PACK_SHRINK);
    box_buttons.pack_start(btn_finish, Gtk::PACK_SHRINK);
    box_buttons.pack_start(btn_cancel, Gtk::PACK_SHRINK);
    box_main.pack_start(box_bottom, Gtk::PACK_SHRINK);

    btn_next.signal_clicked().connect([this] {
      int total = notebook.get_n_pages();

      // If we are on the "Pre-Installation Resume" page (total - 2)
      if (current_page == total - 2) {

        // Calculate the final destination path
        fs::path chosen_path = btn_path_chooser->get_filename();
        if (chosen_path.empty())
          chosen_path = fs::path(getenv("HOME")) / config["DEFINSTDIR"];

        fs::path final_dest = (config["MKDIR"] == "True")
                                  ? chosen_path / config["INSTDIRNAME"]
                                  : chosen_path;

        // Perform the UI check on the MAIN thread
        if (!check_directory_ui(final_dest)) {
          return; // User cancelled or directory error.
        }

        // If user said "go", we go.
        // threaded file stuff >:3 (So we don't lock the UI)
        installDialog = new Gtk::MessageDialog(
            *this, "Installing, please DO NOT close this installer...", false,
            Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE);

        installDialog->show();
        installDialog->set_deletable(false);

        std::thread([this, final_dest] {
          copyApplicationFiles(
              final_dest); // Added a change page statement, this can be
                           // redundant, but necessary in some contexts.
          change_page(1);
          dispatcher.emit();
        }).detach();
      } else {
        change_page(1);
        // I was fed up with notebook's stuff, so, we always update the resume
        // data, regardless.
        update_resume_text(); // yes, this takes up CPU PP&S, sosumi.
      }
    });
    btn_back.signal_clicked().connect([this] { change_page(-1); });
    btn_cancel.signal_clicked().connect([this] { close(); });
    btn_finish.signal_clicked().connect([this] { on_finish(); });
  }

  void change_page(int step) {
    current_page += step;
    notebook.set_current_page(current_page);
    update_page_visibility();
  }

  void update_page_visibility() {
    int total = notebook.get_n_pages();
    btn_back.set_visible(current_page > 0 && current_page < total - 1);
    btn_next.set_visible(current_page < total - 1);
    btn_finish.set_visible(current_page == total - 1);
    btn_next.set_sensitive(current_page == 1 ? chk_accept.get_active() : true);

    if (current_page == total - 2) {
      btn_next.set_label(UIButton_Install);
    } else {
      btn_next.set_label(UIButton_Next);
    }
  }

  // This, is to check if the install path is empty or not.
  bool check_directory_ui(fs::path pathToCheck) {
    // Check if integrity check is even needed, if no? return true so we skip
    // this check, if not...
    if (config["SKIPINTEGRITYCHECK"] == "True")
      return true;
    // check if the directory is empty, and return true if so.
    if (!fs::exists(pathToCheck) || fs::is_empty(pathToCheck))
      return true;

    // if it isn't empty, ask the user if they are SURE they want to risk it.
    Gtk::MessageDialog d(
        *this,
        "The selected installation directory is not empty!\n"
        "Files may be overwritten if you continue. Proceed anyway?",
        false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL);

    int result = d.run();
    return (result == Gtk::RESPONSE_OK); // And finally, return the result :3
  }

  // I forgot if i had this on my last commit, but, this basically is the file
  // copy thread. All this does, is copy stuff... and repeat a few checks for
  // redundancy sake...

  // It may be a waste of CPU in a few cases, but, better safe than sorry.
  // Removed unused BOOLEAN value from function call.
  int copyApplicationFiles(fs::path final_path) {
    try {
      fs::create_directories(final_path);
      if (fs::exists("Binary"))
        fs::copy("Binary", final_path,
                 fs::copy_options::recursive |
                     fs::copy_options::overwrite_existing);
      if (config["HASREADME"] == "True" && fs::exists(config["READMEFILE"]))
        fs::copy(config["READMEFILE"], final_path,
                 fs::copy_options::overwrite_existing);

      // If we checked stuff from the features checker (in dev feature.)
      if (feature_model) {
        int idx = 0;
        for (auto row : feature_model->children()) {
          if (row[feature_columns.col_enabled]) {
            fs::path src =
                fs::path(config["OPTINDIR"]) / features[idx].source_path;
            fs::path tgt;
            if (features[idx].category == "Extra Services")
              tgt = "/etc/systemd/system";
            else if (features[idx].category == "Included Documentation")
              tgt = "/usr/share/man/man1";
            else
              tgt = final_path;

            if (fs::exists(src)) {
              try {
                fs::create_directories(tgt);
                fs::copy(src, tgt / src.filename(),
                         fs::copy_options::recursive |
                             fs::copy_options::overwrite_existing);
              } catch (...) {
                std::cerr << "Permission denied for: " << tgt << std::endl;
                // Dialog saying that the operation could not be completed.
                Gtk::MessageDialog d(
                    *this,
                    "Failed to install feature: " + features[idx].name +
                        "\nPermission denied for: " +
                        std::string(fs::path(tgt)),
                    false, Gtk::MESSAGE_ERROR);
                d.run();
              }
            }
          }
          idx++;
        }
      }
    } catch (const std::exception &e) {
      Gtk::MessageDialog d(*this,
                           "An error occurred when copying files: " +
                               std::string(e.what()),
                           false, Gtk::MESSAGE_ERROR);
      d.run();
    }
    // change_page(1); // Wrong call location.
    return 0;
  }

  void on_finish() {
    // Recalculate the absolute installation path
    fs::path chosen_path = btn_path_chooser->get_filename();
    if (chosen_path.empty()) {
      chosen_path = fs::path(getenv("HOME")) / config["DEFINSTDIR"];
    }
    fs::path final_dest = (config["MKDIR"] == "True")
                              ? (chosen_path / config["INSTDIRNAME"])
                              : chosen_path;

    // Ensure we have an absolute path to the installed files
    fs::path absolute_install_path = fs::absolute(final_dest);
    if (chk_read_me->get_active()) {
      fs::path readme_path = absolute_install_path / config["READMEFILE"];
      // This opens the readme file using xdg-open... might not work in all
      // contexts if Wayland is involved.
      system(("xdg-open \"" + readme_path.string() + "\" &").c_str());
    }

    if (chk_open_app->get_active()) {
      // Remove leading slashes from MAINBINNAME if they exist to avoid
      // root-relative path errors
      std::string bin_name = config["MAINBINNAME"];
      if (!bin_name.empty() && bin_name[0] == '/')
        bin_name = bin_name.substr(1);

      fs::path binary_path = absolute_install_path / bin_name;

      // If the binary is missing... we return a error message and exit with a
      // exception. This is dumb and very badly formatted. But it works
      // regardless.
      if (!fs::exists(binary_path)) {
        Gtk::MessageDialog d(
            *this,
            "Error: Binary does not exist on install directory, was it on the "
            "'Binary' directory relative to setup's executable?\n\nThis might "
            "be caused by a missing file inside the 'Binary' directory on the "
            "setup-program's root directory.\n\n"
            "If you think this is a mistake, we recommend you to re-run this "
            "setup utility, or to manually copy the file referenced by "
            "MAINBINNAME inside the setup.conf file.\n\nOur Most Sincerest "
            "Apologies For The Inconvenience",
            false, Gtk::MESSAGE_ERROR);
        d.run();
        // Exit with error, because if we can't find the binary, we cannot
        // launch the application.
        exit(1);
      } else {
        // If file does indeed exists, execute it.
        // Execute using the full absolute path
        std::string cmd = "\"" + binary_path.string() + "\" &";
        system(cmd.c_str());
      }
    }
    close();
  }
};

// Main Function Entry Point
int main(int argc, char *argv[]) {
  auto app =
      Gtk::Application::create(argc, argv, "com.lds_softworks.setup_wizard");
  RetroInstaller installer;
  return app->run(installer);
}
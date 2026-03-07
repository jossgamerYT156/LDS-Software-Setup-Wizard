#ifndef SETUPWIZARD_HPP
#define SETUPWIZARD_HPP

// Start Of: Static strings and values
// Corporate told me to not hardcode these... i hardcoded them anyways. fight me >:3
// - Lilly Aizawa @ LDS Softworks LLC
#define InstallerVersion        "Version 0.0.23"

#define ProgramWindowName       "Setup Wizard"

// EULA Agreement message
#define AcceptLicenseAgreement  "I accept the terms in the End User License Agreement"

// Setup Configuration File
#define SetupConfigFile         "setup.conf"

// UI Buttons
#define UIButton_Back           "< Back"
#define UIButton_Next           "Next >"
#define UIButton_Install        "Install"
#define UIButton_Finalize       "Finish"
#define UIButton_Cancel         "Cancel"

// Embedded CSS for GTK to keep the Win-95 Style... -ish.
const char win95_css[] =
            "* { "
            "  font-family: 'MS Sans Serif', 'Tahoma', sans-serif; "
            "  font-size: 11px; "
            "  -GtkHTML-fixed-font-size: true; "
            "} "
            "window, .background { "
            "  background-color: #c0c0c0; "
            "  color: black; "
            "} "
            "button { "
            "  background-image: none; "
            "  background-color: #c0c0c0; "
            "  color: black; "
            "  border-radius: 0; "
            "  border: 2px solid; "
            "  border-color: #ffffff #808080 #808080 #ffffff; "
            "  padding: 2px 10px; "
            "  box-shadow: none; "
            "} "
            "button:active, button:checked { "
            "  border-color: #808080 #ffffff #ffffff #808080; "
            "  padding: 3px 9px 1px 11px; "
            "} "
            "button:hover { "
            "  background-color: #c0c0c0; "
            "} "
            "notebook { "
            "  background-color: #c0c0c0; "
            "  border: none; "
            "} "
            "entry, treeview, textview, scrolledwindow { "
            "  background-color: white; "
            "  color: black; "
            "  border-radius: 0; "
            "  border: 2px solid; "
            "  border-color: #808080 #ffffff #ffffff #808080; "
            "} "
            "separator { "
            "  min-height: 2px; "
            "  background-color: transparent; "
            "  border-top: 1px solid #808080; "
            "  border-bottom: 1px solid #ffffff; "
            "} "
            "checkbutton check { "
            "  border-radius: 0; "
            "  border: 2px solid; "
            "  border-color: #808080 #ffffff #ffffff #808080; "
            "  background-color: white; "
            "  box-shadow: none; "
            "  min-height: 12px; "
            "  min-width: 12px; "
            "} "
            "checkbutton check:checked { "
            "  background-image: url('data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iOCIgaGVpZ2h0PSI4IiB2aWV3Qm94PSIwIDAgOCA4IiBmaWxsPSJub25lIiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciPjxwYXRoIGQ9Ik0xIDRMMyA2TDcgMiIgc3Ryb2tlPSJibGFjayIgc3Ryb2tlLXdpZHRoPSIyIi8+PC9zdmc+'); "
            "  background-repeat: no-repeat; "
            "  background-position: center; "
            "}";

// End Of: Static strings and values
#endif
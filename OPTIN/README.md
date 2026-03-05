# **OPT IN Features**
__
This directory, allows you(The developer) to ship extra features such as Systemd service files, extra hardware, documentation, or others, by specifying it in the [Setup Configuration File](../setup.conf) the Values for the ExtraFeatures or OptInFeatures, as well as where the file for the [Features File](features.conf) is located.

This can be customized via the main configuration file, and it is up to the developer to ship these features.

___
## Categories
___
The opt-in features list, has three categories that the installer can handle automatically for you(available as a preset on the provided .conf file with toml syntax.)

These category items more often than not are syntaxed as following:
```toml
 - Item Name: fileName
```

But if these are not available for you, the accepted categories are the following

### Extra Software

This provides the installer a list of extra software to be copied to the root of your installation directory of the software.
Example Syntax:
```toml
Extra Software:
 - Super Cool Extra Software: binaryFileName
```
This must include:
- Display Name (Name that will show in the installer UI)
- File Name to copy (The real file name inside the Opt-In Directory.)

### Extra Services
This category allows for you to include systemd service files inside your software, that of course if your software depends on them.

Example Syntax:
```toml
Extra Services:
 - Required Software Service: example.service
```
> Note, the Installer does NOT enable these services by default.

### Included Software Documentation
This category, allows you to ship man-docs into the installer, making so you can add man-entries of the installed software.

Example Syntax:
```toml
Included Documentation:
 - Useful Software Documentation: useful_docs.pdf
```

> Note, it is recommended to add this entry if your software does not provide a GUI.


# DISCLAIMER
LDS Softworks LLC Does NOT makes itself responsible by Developer Misuse of this feature.
YOU(The User) should ALWAYS review what features are going to be installed, as well as ensure that what you're installing is what you wanted.
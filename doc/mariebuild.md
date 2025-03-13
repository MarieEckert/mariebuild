# Mariebuild Overview
## Commandline Usage
**Synposis**
```
mb [-i <mariebuild file>] [-fkn] [-v 0-3] [-t <target name>]
```

### Options
| Short   | Long      | Description |
| -----   | --------- | ----------- |
| -i FILE | --in=FILE | Specify which file to use as the buildfile. If not provided, mariebuild defaults to build.mb in the current working directory |
| -f      | --force   | Build every file, even if in incremental mode |
| -k      | --keep-going | Ignore errors which occured whilst building and continue on (if possible) |
| -n      | --no-splash | Do not print the mariebuild splash screen |
| -v LEVEL | --verbosity=LEVEL | Set the logging verbosity level (0-3; 0 prints everything from debug and up; 3 is only errors) |
| -t TARGET | --target=TARGET | Set the target to build. If not provided mariebuild will use the provided default target. If no default target is specified, it will try to run the debug target |
| -? | --help | Display a help text for mariebuild |
| -V | --version | Display version information about mariebuild |

## File structure
Mariebuild utilises the MCFG/2 format for its build files. These are structured into sectors, then sections, then fields. Fields may only be declared within sections, which intern can only be declared within sectors.

Mariebuild requires 2-3 sectors:

1. config
2. targets
3. c_rules (if wanted)

For information about targets and c_rules, see their respective files within the documentation.

### The Config Sector
Within the config sector, mariebuild expects a mariebuild section containing a list of targets which should be available through the command line. A Default target can also be declared here.

Several additional fields can also be declared here, which will be used by mariebuild:

* `build_type` (str) – The default build type for each rule.
* `default_log_level` (u8) – The log level which should be used by default, overriden by the `-v` command line argument.

Example:
```mcfg2
sector config
  section mariebuild
    ; build_type can either be incremental or full
    str build_type 'incremental'

    list str targets 'clean', 'debug', 'release'
    str default 'debug'
  end
end
```

## Relevant Source-Files
```
src/
    main.c
    build.c
    build.h
```

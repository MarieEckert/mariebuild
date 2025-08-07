# Mariebuild
A build system primarly designed and intended for my use.

## Building
### Setup
To quickly setup mariebuild for building, simply run the `setup.bash` script. This will
automatically build [libmcfg_2](https://git.heroin.trade/marie/mcfg_2) from the tag/commit specified in the script.

### Compiling
**With mariebuild (0.5.0 or higher):** `mb -t release` <br>
**Without mariebuild:** `bash build.bash`

## mb usage
By default mb looks for a `build.mb` file which is the executed in debug mode.
```
Usage: mb [OPTION...]
A simple build system inspired by my hate against makefiles
Author: Marie Eckert

  -f, --force                Force a build, regardless if target is
                             incremental
  -i, --in=FILE              Specify a buildfile
  -k, --keep-going           Ignore any failures (if possible) and keep on building
  -n, --no-splash            Disable splash screen/logo
  -t, --target=TARGET        Specify the build target
  -v, --verbosity=LEVEL      Set the verbosity level (0-3)
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to https://git.heroin.trade/marie/mariebuild/issues.
```

## TODO (for mariebuild 1.0.0)

- [X] Move to MCFG/2
- [X] Incremental and Full Builds
- [X] Unify and Singular Rules
- [X] Implementation of the "force" argument
- [X] Target-Dependant Fields
- [ ] Documentation

## License
Mariebuild is licensed under the BSD 3-Clause License, see [The LICENSE file](https://git.heroin.trade/marie/mariebuild/src/branch/trunk/LICENSE).

## Links

* [radicle mirror](https://app.radicle.xyz/nodes/seed.radicle.garden/rad:zXKPxNiqRY8bCHWK6XLdJLRRCjai)

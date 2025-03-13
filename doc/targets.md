# Mariebuild Targets

Targets are a central concept of mariebuild, everytime you build something with mariebuild
you are building a target. Targets can require other targets to be built before themselves and
can also require c_rules to be fulfilled. Additionally targets can have their own script which
is executed after all required targets are built and all c_rules are fulfilled.

Each target must be declared as a section within the `targets` sector.

## Build Order

The order of how things are executed/targets built/c_rules fulfilled is the following:

1. required targets of a target
2. required c_rules of a target  
2.1. required c_rules of a c_rule  
2.2. the c_rule itself
3. the target itself (the `exec` script)

## Requiring other Targets

To make one target require other targets, simply add a string list to the target containing
the names of each required target.

It is important to note that as soon as mariebuild detects a circular target requirement,
the build is aborted (Though a history of targets which have been "visited" is given in this case).

### Example

```mcfg2
; [...]
sector targets
    section foo
        ; [...]
    end

    section bar
        ; [...]
    end

    section default
        list str required_targets 'foo', 'bar'
    end
end
; [...]
```

## Requiring c_rules

To require one or multiple c_rules for a target is also accomplished through a string list of
names. The same circular-requirement rules which apply for targets also apply for c_rules.

```mcfg2
; [...]
sector targets
    section default
        list str c_rules 'foo'
    end
end

sector c_rules
	section foo
		; [...]
	end
end
```

## Target-Dependant Fields

It may be practical to have a field which holds a different value depending on which target is currently being built. This is accomplished through MCFG/2 dynfields by linking every field of which the name begins with `target_` within a target section to a newly created dynfield with the same name.

This can be used to, for example, write object files for debug builds to a different directory than object files for release builds.

### Example


```mcfg2
; [...]
sector targets
    section default
        list str c_rules 'foo'

        str target_objdir 'debug/obj'
    end
end

sector c_rules
	section foo
		; [...]
		str exec 'clang -c -o $(%target_objdir%)$(%output%) $(%input%)`'
	end
end
```

**It may also be useful to read the [MCFG/2 Documentation](https://github.com/MarieEckert/mcfg_2/blob/main/doc/embed_formatting.md) on field embedding/formatting**

## Relevant Source Files
```
src/
    target.c
    target.h
    executor.c
    executor.h
```

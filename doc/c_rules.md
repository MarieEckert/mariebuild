# Mariebuild c_rules

c_rules (originally "compilation rules") are intended to be used to contain work required to be done by multiple targets or wrap tasks which have multiple input files which have to be either combined or processed individually.

c_rules offer up the following possibilities:

1. Combining multiple inputs into one output
2. Processing multiple inputs separately
	1. Doing this incrementally (only processing inputs which are newer than their output)
	2. Parallel execution / processing of the inputs (no excluded by 2.1)
	

## Specifying inputs & outputs

each c_rule must posses a list of inputs in the form of a string list or a path to a string list. Optionally an explicit list of outputs with the same length as the input  list can be specified, if this list is not specified it will be the same as the input list.

Additionally, formatting for both inputs and outputs must be specified. This formatting is applied to each element in the list before it is processed via the `exec` script.

### Example

```mcfg2
; [...]
sector foo
	section bar
		list str other_input 'a', 'b', 'c', 'd'
	end
end

sector c_rules
	section foo
		; List inputs
		list str input 'main', 'log', 'display'
		
		; OR provide a path to a list of inputs
		; str input_src '/foo/bar/other_input'
		
		; Optional list of output names for each input element
		; list str output 'x', 'y', 'z'
		
		; OR provide a path to a list of output names
		; str output_src '/foo/bar/other_input'
		
		; Specify how the input and output elements should be formatted.
		; The element dynfield which is embedded here will resolve to whatever
		; element is currently being processed.
		str input_format 'src/$(%element%).c'
		str output_format 'obj/$(%element%).o'
		
		; When executing this will run the following commands one after another:
		; clang -c -o obj/main.o src/main.c
		; clang -c -o obj/log.o src/log.c
		; clang -c -o obj/display.o src/display.c
		str exec 'clang -c -o $(%output%) $(%input%)'
	end
end
```

## "Unify" c_rules

Unify c_rules are c_rules which take multiple input elements and combine them into one output element. This is most commonly used for linking executables.

For these c_rules only a list of input elements is relevant but the value of the `output` dynfield is still determined by the value of the `output_format` field. You should not use the `element` dynfield within the `output_format` field.

### Example

```mcfg2
section executable
	str exec_mode 'unify'

	str input_src '/config/files/sources'

	str input_format '$(%target_objdir%)$(%element%).o'
	str output_format '$(%target_builddir%)$(/config/files/binname)'

	str exec 'clang -o $(%output%) $(%input%)'
```

## "Singular" c_rules

Singular c_rules are c_rules which process each input for itself. Meaning that one input corresponds to one output. These require both a list of input and output elements.

For an example, see the example in the "Specifying inputs & outputs" section.

### Parallel Processing

It is also possible to process multiple inputs in parallel by simply adding a boolean type field called `parallel` to the c_rule you wish to apply this to and setting it to true.

By default this caps out at 8 processes but you can change this limit adding a u8 field called `max_procs`.

#### Example

```mcfg2
; [...]
sector c_rules
	section foo
		bool parallel true
		; Optionally adjust the number of maximum parallel processes
		; u8 max_procs 16
		
		; [...]
	end
end
```

## Incremental vs. Full building

c_rules can have two so called "build_types":

1. incremental
2. full

Incremental c_rules only process inputs which are newer than their corresponding outputs. Full c_rules always process every input they have.

If a c_rule doesn't specify which build type it uses, it defaults to the build type specified in the /config/mariebuild section. If this is also not specified it will be incremental by default.

The build type is set by declaring a string field called `build_type` within the c_rule or the mariebuild config.

## Relevant Source Files
```
src/
    c_rule.c
    c_rule.h
    executor.c
    executor.h
```

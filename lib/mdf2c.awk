#
# Awk script for generating a .c file from a .mdf file
#

BEGIN {
    printf("/* Automatically generated - may need to edit! */\n");
    printf("\n");
    printf("#include <dx/dx.h>\n");
    printf("#include <dx/modflags.h>\n");
    printf("\n");
    printf("#if defined(intelnt) || defined(WIN32)\n");
    printf("#include <windows.h>\n");
    printf("#endif\n");
    printf("\n");
    printf("#if defined(__cplusplus)\n");
    printf("extern \"C\" Error DXAddModule (char *, ...);\n");
    printf("#else\n");
    printf("extern Error DXAddModule (char *, ...);\n");
    printf("#endif\n");
    printf("\n");
    firsttime = 1
}

# for run-time loadable modules, dynamic is set to 1 by the calling script
{   if (firsttime > 0) {
        if (dynamic > 0) {
        printf("#if defined(__cplusplus)\n");
        printf("extern \"C\" Error m_%s(Object*, Object*);\n",$2);
        printf("#endif\n");
        printf("#if defined(intelnt) || defined(WIN32)\n");
        printf("void FAR WINAPI DXEntry()\n");
        printf("#else\n");
        printf("  #if defined(__cplusplus)\n");
        printf("    extern \"C\" void DXEntry()\n");
        printf("  #else\n");
        printf("    void DXEntry()\n");
        printf("  #endif\n");
        printf("#endif\n");
        }
        else
            printf("void\n _dxf_user_modules()\n");
        printf("{\n");
        firsttime = 0;
    }
}

# comments in the .mdf file start with '#' in the first column

/^#/ {
    next
}

# output the previous module definition when the next module definition starts 
# (for the first module definition, there is no previous module definition,
#  and at the end of file, the last module definition must be output.)

/^MODULE/ {
    if (module!="") {
	printf("    {\n")
	printf("#if defined(__cplusplus)\n")
	printf("        extern \"C\" Error %s(Object *, Object *);\n", funcname)
	printf("#else\n")
	printf("        extern Error %s(Object *, Object *);\n", funcname)
	printf("#endif\n")
        printf("        DXAddModule(\"%s\", %s, ", module, funcname)
	if (flags=="")
	    printf("0")
	else
	    printf("\n            %s", flags)
	printf(",\n            %d", ninputs + input_repeat*20)
	for (i=0; i<ninputs; i++)    
	    printf(", \"%s%s\"", inputs[i], inputattrs[i])
	for (i=0; i<20; i++)
	    for (j=0; j<input_repeat; j++) {
		k = ninputs - input_repeat + j
	        printf(", \"%s%d%s\"", inputs[k], i+1, inputattrs[k])
            }
	printf(",\n            %d", noutputs + output_repeat*20)
	for (i=0; i<noutputs; i++)
	    printf(", \"%s%s\"", outputs[i], outputattrs[i])
	for (i=0; i<20; i++)
	    for (j=0; j<output_repeat; j++) {
		k = noutputs - output_repeat + j
	        printf(", \"%s%d%s\"", outputs[k], i+1, outputattrs[k])
            }
	if (outboard)
	    printf(",\n            \"%s\", \"%s\"", outboard_exec, outboard_host);
	printf(");\n");
	printf("    }\n");
    }

    # reset these for next time

    ninputs = 0
    noutputs = 0
    input_repeat = 0
    output_repeat = 0
    flags = ""
    outboard = 0
    outboard_exec = ""
    outboard_host = ""

    module = $2
    funcname = "m_" $2
}

# input parameters.  each input is on a separate line.  if there are
# input attributes, they are parsed so spaces can be squeezed out.

/^INPUT/ {
    # split input line at semicolons, INPUT parmname should be first phrase
    ns = split($0, a, ";");

    # split again at spaces, which should separate INPUT from parmname
    ns = split(a[1], b);

    # if there are attributes following the parmname, squeeze any spaces
    # out of them so the input parm is 'parmname[attr:value,attr:value]'
    c = ""
    for (i=2; i<=ns; i++)
      c = c b[i];

    # now split again on [ to separate name from attributes
    ns = split(c, d, "[");
    if (d[2] != "")
	d[2] = "[" d[2]

    inputs[ninputs] = d[1]
    inputattrs[ninputs++] = d[2]
}

# output parameters.  each output is on a separate line.  if there are
# output attributes, they are parsed so spaces can be squeezed out.

/^OUTPUT/ {
    # split output line at semicolons, OUTPUT parmname should be first phrase
    ns = split($0, a, ";");

    # split again at spaces, which should separate OUTPUT from parmname
    ns = split(a[1], b);

    # if there are attributes following the parmname, squeeze any spaces
    # out of them so the output parm is 'parmname[attr:value,attr:value]'
    c = ""
    for (i=2; i<=ns; i++)
      c = c b[i];

    # now split again on [ to separate name from attributes
    ns = split(c, d, "[");
    if (d[2] != "")
	d[2] = "[" d[2]

    outputs[noutputs] = d[1]
    outputattrs[noutputs++] = d[2]
}

# optional module flags.
# flag keywords are all space separated, and just need MODULE_ prepended
# to make them match the modflags.h file definitions for a bitmask.

/^FLAGS/ {
    for (i=2; i<=NF; i++)
        if (flags != "")
	    flags = flags " | MODULE_" $i
        else
	    flags = "MODULE_" $i
}

# only valid for outboard modules (modules in separate executables, as 
# contrasted with normal modules which are linked directly into the exec).
# the name of the executable file to be run is required; an optional hostname
# to run on can be specified.

/^OUTBOARD/ {
    outboard = 1
    funcname = "DXOutboard"

    # force the OUTBOARD flag to be set
    if (flags != "")
        flags = flags " | MODULE_OUTBOARD"
    else
        flags = "MODULE_OUTBOARD"

    # if there is a trailing hostname, find it first
    ns = split($0, a, ";")
    if (ns > 1) {
        split(a[2], b, " ");
        outboard_host = b[1]
    }

    
    # this finds either a string in quotes or a single token
    # in the new version of awk only.  the match() function
    # doesn't exist on some of the other supported platforms

    #if (match(a[1], "\".+\"") > 0)
    #    outboard_exec = substr($0, RSTART+1, RLENGTH-2)
    #else {
    #    split(a[1], b, " ")
    #    outboard_exec = b[2]
    #}

    # attempt at straight awk replacement code
    if (a[1] ~ /\".+\"/) {
	ns = split(a[1], b, "\"")
	outboard_exec = b[2]
	for (i=3; i<ns; i++)
	    outboard_exec = outboard_exec "\"" b[i]
    } else {
	split(a[1], b, " ")
	outboard_exec = b[2]
    }
}

# only valid for inboard modules (modules which are set up to be dynamically
# loaded into the running exec after it has started).
# the name of the executable file to be run is required, but the user.c
# file doesn't need this name - it just needs to skip this line.

/^INBOARD/ {
    next
}


# repeated inputs or outputs.  the default repeat count is 1.
# if this follows the input lines, this indicates repeated inputs.
# if this follows the output lines, this indicates repeated outputs.

/^REPEAT/ {
    if (NF>=2)
	count = $2+0
    else
	count = 1
    if (noutputs==0)
	input_repeat = count
    else
	output_repeat = count
}

# output last module definition.  this code must match the code in the
# /^MODULE/ section (plus a final '}' to end the subroutine.)

END {
    if (module!="") {
	printf("    {\n")
	printf("#ifndef __cplusplus\n")
	printf("        extern Error %s(Object *, Object *);\n", funcname)
	printf("#endif\n")
        printf("        DXAddModule(\"%s\", %s, ", module, funcname)
	if (flags=="")
	    printf("0")
	else
	    printf("\n            %s", flags)
	printf(",\n            %d", ninputs + input_repeat*20)
	for (i=0; i<ninputs; i++)    
	    printf(", \"%s%s\"", inputs[i], inputattrs[i])
	for (i=0; i<20; i++)
	    for (j=0; j<input_repeat; j++) {
		k = ninputs - input_repeat + j
	        printf(", \"%s%d%s\"", inputs[k], i+1, inputattrs[k])
            }
	printf(",\n            %d", noutputs + output_repeat*20)
	for (i=0; i<noutputs; i++)
	    printf(", \"%s%s\"", outputs[i], outputattrs[i])
	for (i=0; i<20; i++)
	    for (j=0; j<output_repeat; j++) {
		k = noutputs - output_repeat + j
	        printf(", \"%s%d%s\"", outputs[k], i+1, outputattrs[k])
            }
	if (outboard)
	    printf(",\n            \"%s\", \"%s\"", outboard_exec, outboard_host);
	printf(");\n");
	printf("    }\n");
    }

    printf("}\n");
}

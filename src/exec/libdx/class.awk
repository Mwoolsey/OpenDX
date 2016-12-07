#
# The "class vector" is a list of static (read-only) class information.
# Each class vector begins with a copy of its super-class class vector (if
# any; e.g., class "object" has no super-class vector).  The next two entries
# identify the subclass by id and name, in case this class gets subclassed.
# (If not, these just contain the current class id and name.)  Finally, there
# is one entry per method DEFINEd by this class.
#
# We keep track of a separate class value vector for each class.  Initially,
# it is the same as its superclass class value vector, with the addition of
# a subclass id and name.  For each method DEFINEd by the class, a new value
# is added to the class value vector to point to the method implementation.
# For each method IMPLEMENTed by the class, the value of the superclass
# method is replaced in the class value vector.
#
# classes[i]			name of ith class
# do_class_c[i]                 was do_c in effect for ith class?
# nclasses			number of classes so far
# super_names[i]		name of super class of ith class (0 if none)
#
# class_vector[class i]		ith class value for given class
# method_name[class i]		method name of ith class value for given class
# method_number[class method]	position of given method of given class
# first[class]			first class value that corresponds to class
# nvalues[class]		number of class values for given class
#

BEGIN {
    nclasses = 0
    alpha = "abcdefghijklmnopqrstuvwxyz"
    ALPHA = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    do_h = 0
    do_c = 0
    printf("/*\n")
    printf(" * Automatically generated - DO NOT EDIT!\n")
    printf(" */\n")
    printf("\n")
    nop = "_dxfno_" # prefix for unimplemented method names
    class_overhead = 2		# extra info per class vector structure
    root_overhead = 3		# additional extra info for root class
				# change this to non-zero to enable sizes
				# this is the number of entries in struct _root
}

/^#do_h$/ {
    do_h = 1
    next
}

/^#do_c$/ {
    do_c = 1
    next
}

/^INCLUDE/ {
    next
}

/^CLASS/ || /^SUBCLASS.* OF / {

    #
    # Which class we're defining
    #
    class = $2
    do_class_c[nclasses] = do_c
    classes[nclasses] = class
    nclasses++
    if (NF==4) super = $4; else super = 0
    super_names[class] = super

    #	
    # Compute upper and lower case versions of name
    #
    n = length(class);
    uc = lc = ""
    for (i=1; i<=n; i++) {
	c = substr(class, i, 1)
	uu = index(ALPHA, c)
	ll = index(alpha, c)
	if (ll) uc = uc substr(ALPHA, ll, 1); else uc = uc c
	if (uu) lc = lc substr(alpha, uu, 1); else lc = lc c
    }
    uc_names[class] = uc
    lc_names[class] = lc

    if (super) {

	#
	# copy superclass class vector,
	#
	f = first[class] = nvalues[super]
        for (i=0; i<f; i++) {
	    class_vector[class i] = class_vector[super i]
	    method_name[class i] = method = method_name[super i]
	    method_number[class method] = i
        }

	#
	# except override first class_overhead values of superclass
        # (CLASS_ and "class") and since we aren't subclassed yet,
	# copy those values as the first values of our new vector
	#
        fs = first[super]
        class_vector[class f] = class_vector[class fs] = "CLASS_"uc
        class_vector[class f+1] = class_vector[class fs+1] = "\""lc"\""
        nvalues[class] = f + class_overhead

    } else {

	#
	# We're a root class - skip the first root_overhead entries
	# and put our class_overhead entries in
	#
        f = first[class] = root_overhead
        class_vector[class f] = "CLASS_"uc
        class_vector[class f+1] = "\""lc"\""
        nvalues[class] = f + class_overhead

	#
	# Every root class vector begins with this structure
	# Change root_overhead if you change this
	#
	if (do_h) {
	    printf("struct _root {\n");
	    printf("    int size;\n")
	    printf("    Class class;\n")
	    printf("    char *name;\n")
	    printf("};\n")
	    printf("#define CLASS_SIZE(x) (((struct _root *)(x))->size)\n")
	    printf("#define CLASS_CLASS(x) (((struct _root *)(x))->class)\n")
	    printf("#define CLASS_NAME(x) (((struct _root *)(x))->name)\n")
	    printf("\n");
	}
    }
	
    # override size (part of root_overhead)
    if (root_overhead>0) {
	class_vector[class 0] = "sizeof(struct "lc")"
	class_vector[class 1] = "CLASS_"uc
	class_vector[class 2] = "\""lc"\""
    }

    if (do_h)
	printf("\nextern struct %s_class _dxd%s_class;\n", lc, lc);

    next
}

/^IMPLEMENTS/ {

    for (i=2; i<=NF; i++) {

	#
	# Our class implements method given by $i itself
	#
        method = $i
        n = method_number[class method]
        class_vector[class n] = "_dxf" class "_" method

        if (do_h) {
	    #
	    # Declare our implementation
	    #
            printf("%s _dxf%s_%s(", returns[method], class, method)
            na = nargs[method]
	    # note we change type of first arg
            if (na>0)
                printf("%s", class)
            for (k=1; k<na; k++)
                printf(",%s", argtype[method k])
            printf(");\n")
	}
    }

    next
}


/^DEFINES/ {

    #
    # Our class defines a new method.  First,
    # parse the method declaration
    #
    split($0, aaa, "(")
    split(aaa[1], bbb, " ")
    type = bbb[2]
    method = bbb[3]
    split(aaa[2], ccc, ")")
    na = split(ccc[1], args, ",")

    #
    # Add it to our class vector
    #
    nv = nvalues[class]
    method_number[class method] = nv
    method_name[class nv] = method
    class_vector[class nv] = nop method
    nvalues[class]++

    #
    # Record the return type, and number
    # and types of arguments
    #
    returns[method] = type
    nargs[method] = na
    for (i=1; i<=na; i++)
        argtype[method (i-1)] = args[i]

    #
    # Add method declaration to the .h file
    # Normally this will have been declared elsewhere;
    # this provides additional checking
    #
    if (do_h) {
	printf("%s _dxf%s(", type, method)
        if (na>0)
           printf("%s", argtype[method 0])
        for (k=1; k<na; k++)
           printf(",%s", argtype[method k])
        printf(");\n")	

	#
	# Add _no_ method declaration for use by subclasses
	#

	printf("%s %s%s(", type, nop, method)
        if (na>0)
            printf("%s", argtype[method 0])
        for (k=1; k<na; k++)
            printf(",%s", argtype[method k])
        printf(");\n")	
    }

    next    
}

!/^[ \t]*\/\// {
    #
    # Copy anything that we don't recongize to the .h file
    #
    if (do_h)
        print $0
    next
}

END {

    if (do_c) {

	print ""

	#
	# For each class, including superclasses,
	#
        for (i=0; i<nclasses; i++) {

            class = classes[i]
	    lc = lc_names[class]
	    uc = uc_names[class]
            nv = nvalues[class]
	    super = super_names[class]
            f = first[class] + class_overhead

	    #
	    # Print out the class vector declaration
	    #
            printf("struct %s_class {\n", lc)
            if (super_names[class])
                printf("    struct %s_class super;\n", lc_names[super])
	    else if (root_overhead>0)
		printf("    struct _root root;\n")
            printf("    Class class;\n")
            printf("    char *name;\n")
            for (j=f; j<nv; j++) {
                method = method_name[class j]
                printf("    %s (*%s)(", returns[method], method)
                printf(");\n")
            }
            printf("};\n")
	    printf("\n")

	    #
	    # Do implementation routines etc. only if do_c was in
	    # effect when we first saw this class.  Because of the
	    # way the class script sets do_c, this allows us to
	    # distinguish between the base .class file and the .class
	    # files it INCLUDEs.  This is necessary so we define the
	    # methods, class vectors, etc. only once, not each time
	    # the .class file is INCLUDEd in each subclass.
	    # 
	    if (!do_class_c[i])
		continue

	    #
	    # Create a "not implemented" routine for each method
	    #
	    for (j=f; j<nv; j++) {
		method = method_name[class j]
		na = nargs[method]
	        printf("%s %s%s(", returns[method], nop, method);
	        if (na>0)
	            printf("%s %s", argtype[method 0], substr(alpha,1,1))
	        for (k=1; k<na; k++)
	            printf(",%s %s", argtype[method k], substr(alpha,k+1,1))
	        printf(") {\n")
	        printf("    char *name;\n")
	        printf("    name = (*(struct %s_class **)a)->name;\n", lc_names[class])
 		printf("    DXSetError(ERROR_BAD_PARAMETER, \"#12130\", \"DX%s\", name);\n", method);
                if (returns[method] ~ /Error/)
	           printf("    return ERROR;\n")
                else
		   printf("    return NULL;\n")
	        printf("}\n")
	        printf("\n")
	    }

	    #
	    # Create and initialize the class vector itself
	    #
            printf("struct %s_class _dxd%s_class = {\n", lc, lc)
            for (j=0; j<nv; j++)
                printf("    %s,\n", class_vector[class j])
            printf("};\n")

	    #
	    # Print out the method implementations, which just
	    # use the class vector to call the right method
	    #
            for (j=f; j<nv; j++) {
               method = method_name[class j]
               printf("\n")
               printf("%s _dxf%s(", returns[method], method)
               na = nargs[method]
               if (na>0)
                   printf("%s %s", argtype[method 0], substr(alpha,1,1))
               for (k=1; k<na; k++)
                   printf(",%s %s", argtype[method k], substr(alpha,k+1,1))
               printf(") {\n")
	       printf("    if (!a)\n")
	       if (returns[method] ~ /Error/)
		  printf("        return ERROR;\n")
	       else
	          printf("        return NULL;\n")
	       printf("    return (*(struct %s_class **)a)->%s(", lc, method)
	    # note we change type of first arg
               if (na>0)
                   printf("(%s)%s", class, substr(alpha,1,1))
               for (k=1; k<na; k++)
                   printf(",%s", substr(alpha,k+1,1))
               printf(");\n")
               printf("}\n")
            }
            printf("\n")
                
	    #
	    # Define a Get...Class routine for this class
	    #
	    printf("Class DXGet%sClass(%s o) {\n", class, class)
	    printf("    return o? (*(struct %s_class **)o)->class: CLASS_MIN;\n",\
                                                                           lc)
	    printf("}\n")
            printf("\n")
        }
    }
}




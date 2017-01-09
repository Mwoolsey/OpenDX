( char * ) "//\n", ( char * ) "// time: Wed Mar  5 11:15:47 2003\n",
    ( char * ) "//\n", ( char * ) "// version: 3.2.0 (format), 4.2.0 (DX)\n",
    ( char * ) "//\n", ( char * ) "//\n", ( char * ) "// MODULE main\n",
    ( char * ) "//\n",
    ( char * ) "// comment: This example creates banded colors for a 2 "
               "dimensional data "
               "set. The interactors allow you to specify the break points of "
               "the bands, "
               "as well as the set of colors to apply to the bands. \n",
    ( char * ) "// page assignment: java seqno	order=2, windowed=0, "
               "showing=1\n",
    ( char * ) "// workspace: width = 565, height = 522\n",
    ( char * ) "// layout: snap = 0, width = 50, height = 50, align = NN\n",
    ( char * ) "//\n", ( char * ) "macro main(\n", ( char * ) ") -> (\n",
    ( char * ) ") {\n", ( char * ) "    // \n",
    ( char * ) "    // node GetLocal[1]: x = 166, y = 10, inputs = 3, label = "
               "GetLocal\n",
    ( char * ) "    // input[1]: defaulting = 0, visible = 1, type = 67108863, "
               "value = "
               "0\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "main_GetLocal_1_out_1,\n",
    ( char * ) "main_GetLocal_1_out_2 = \n", ( char * ) "    GetLocal(\n",
    ( char * ) "    main_GetLocal_1_in_1,\n",
    ( char * ) "    main_GetLocal_1_in_2,\n",
    ( char * ) "    main_GetLocal_1_in_3\n",
    ( char * ) "    ) [instance: 1, cache: 1];\n", ( char * ) "    // \n",
    ( char * ) "    // node Compute2[1]: x = 10, y = 190, inputs = 5, label = "
               "Compute2\n",
    ( char * ) "    // input[1]: defaulting = 0, visible = 1, type = 32, value "
               "= "
               "\"loop+1\"\n",
    ( char * ) "    // input[2]: defaulting = 0, visible = 1, type = 32, value "
               "= "
               "\"loop\"\n",
    ( char * ) "    // input[4]: visible = 0\n",
    ( char * ) "    // input[5]: visible = 0\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "main_Compute2_1_out_1 = \n", ( char * ) "    Compute2(\n",
    ( char * ) "    main_Compute2_1_in_1,\n",
    ( char * ) "    main_Compute2_1_in_2,\n",
    ( char * ) "    main_GetLocal_1_out_1,\n",
    ( char * ) "    main_Compute2_1_in_4,\n",
    ( char * ) "    main_Compute2_1_in_5\n",
    ( char * ) "    ) [instance: 1, cache: 1];\n", ( char * ) "    // \n",
    ( char * ) "    // node GetGlobal[1]: x = 248, y = 100, inputs = 3, label "
               "= "
               "GetGlobal\n",
    ( char * ) "    // input[1]: defaulting = 0, visible = 1, type = 67108863, "
               "value = "
               "0\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "main_GetGlobal_1_out_1,\n",
    ( char * ) "main_GetGlobal_1_out_2 = \n", ( char * ) "    GetGlobal(\n",
    ( char * ) "    main_GetGlobal_1_in_1,\n",
    ( char * ) "    main_GetGlobal_1_in_2,\n",
    ( char * ) "    main_GetGlobal_1_in_3\n",
    ( char * ) "    ) [instance: 1, cache: 1];\n", ( char * ) "    // \n",
    ( char * ) "    // node Compute2[2]: x = 102, y = 190, inputs = 5, label = "
               "Compute2\n",
    ( char * ) "    // input[1]: defaulting = 0, visible = 1, type = 32, value "
               "= "
               "\"loop+1\"\n",
    ( char * ) "    // input[2]: defaulting = 0, visible = 1, type = 32, value "
               "= "
               "\"loop\"\n",
    ( char * ) "    // input[4]: visible = 0\n",
    ( char * ) "    // input[5]: visible = 0\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "main_Compute2_2_out_1 = \n", ( char * ) "    Compute2(\n",
    ( char * ) "    main_Compute2_2_in_1,\n",
    ( char * ) "    main_Compute2_2_in_2,\n",
    ( char * ) "    main_GetGlobal_1_out_1,\n",
    ( char * ) "    main_Compute2_2_in_4,\n",
    ( char * ) "    main_Compute2_2_in_5\n",
    ( char * ) "    ) [instance: 2, cache: 1];\n", ( char * ) "    // \n",
    ( char * ) "    // node Compute2[3]: x = 32, y = 280, inputs = 5, label = "
               "Compute2\n",
    ( char * ) "    // input[1]: defaulting = 0, visible = 1, type = 32, value "
               "= "
               "\"global-local\"\n",
    ( char * ) "    // input[2]: defaulting = 0, visible = 1, type = 32, value "
               "= "
               "\"local\"\n",
    ( char * ) "    // input[4]: defaulting = 0, visible = 1, type = 32, value "
               "= "
               "\"global\"\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "main_Compute2_3_out_1 = \n", ( char * ) "    Compute2(\n",
    ( char * ) "    main_Compute2_3_in_1,\n",
    ( char * ) "    main_Compute2_3_in_2,\n",
    ( char * ) "    main_Compute2_1_out_1,\n",
    ( char * ) "    main_Compute2_3_in_4,\n",
    ( char * ) "    main_Compute2_2_out_1\n",
    ( char * ) "    ) [instance: 3, cache: 1];\n", ( char * ) "    // \n",
    ( char * ) "    // node Compute[1]: x = 75, y = 370, inputs = 3, label = "
               "Compute\n",
    ( char * ) "    // input[1]: defaulting = 0, visible = 0, type = 32, value "
               "= \"[$0, "
               "$1]\"\n",
    ( char * ) "    // page group: java seqno\n",
    ( char * ) "    // expression: value = [a, b]\n",
    ( char * ) "    // name[2]: value = a\n",
    ( char * ) "    // name[3]: value = b\n", ( char * ) "    //\n",
    ( char * ) "main_Compute_1_out_1 = \n", ( char * ) "    Compute(\n",
    ( char * ) "    main_Compute_1_in_1,\n",
    ( char * ) "    main_Compute2_3_out_1,\n",
    ( char * ) "    main_GetLocal_1_out_1\n",
    ( char * ) "    ) [instance: 1, cache: 1];\n", ( char * ) "    // \n",
    ( char * ) "    // node DXLInput[1]: x = 353, y = 10, inputs = 1, label = "
               "_java_control\n",
    ( char * ) "    // input[1]: defaulting = 0, visible = 1, type = 29, value "
               "= 0\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "    main_DXLInput_1_out_1 = _java_control;\n",
    ( char * ) "    // \n", ( char * ) "    // node SetGlobal[1]: x = 249, y = "
                                       "280, inputs = 3, label = "
                                       "SetGlobal\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "    SetGlobal(\n", ( char * ) "    main_Compute2_2_out_1,\n",
    ( char * ) "    main_GetGlobal_1_out_2,\n",
    ( char * ) "    main_SetGlobal_1_in_3\n",
    ( char * ) "    ) [instance: 1, cache: 1];\n", ( char * ) "    // \n",
    ( char * ) "    // node SetLocal[1]: x = 167, y = 280, inputs = 3, label = "
               "SetLocal\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "    SetLocal(\n", ( char * ) "    main_Compute2_1_out_1,\n",
    ( char * ) "    main_GetLocal_1_out_2,\n",
    ( char * ) "    main_SetLocal_1_in_3\n",
    ( char * ) "    ) [instance: 1, cache: 1];\n", ( char * ) "    // \n",
    ( char * ) "    // node Transmitter[3]: x = 57, y = 460, inputs = 1, "
               "label = java_sequence\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "java_sequence = main_Compute_1_out_1;\n",
    ( char * ) "    // \n", ( char * ) "    // node Transmitter[4]: x = 356, y "
                                       "= 100, inputs = 1, label = "
                                       "java_control\n",
    ( char * ) "    // page group: java seqno\n", ( char * ) "    //\n",
    ( char * ) "java_control = main_DXLInput_1_out_1;\n", ( char * ) "    //\n",
    ( char * ) "    // decorator Annotate	pos=(330,206) size=235x40 "
               "style(Label), value = <NULL>\n",
    ( char * ) "    // annotation user_begin: 58\n",
    ( char * ) "    // annotation user: D O   N O T   E D I T\n",
    ( char * ) "    // annotation user: This page was created automatically.\n",
    ( char * ) "    // annotation user_end: <NULL>\n",
    ( char * ) "    // page group: java seqno\n",
    ( char * ) "// network: end of macro body\n", ( char * ) "}\n",
    ( char * ) "main_GetLocal_1_in_1 = 0;\n",
    ( char * ) "main_GetLocal_1_in_2 = NULL;\n",
    ( char * ) "main_GetLocal_1_in_3 = NULL;\n",
    ( char * ) "main_GetLocal_1_out_1 = NULL;\n",
    ( char * ) "main_GetLocal_1_out_2 = NULL;\n",
    ( char * ) "main_Compute2_1_in_1 = \"loop+1\";\n",
    ( char * ) "main_Compute2_1_in_2 = \"loop\";\n",
    ( char * ) "main_Compute2_1_in_4 = NULL;\n",
    ( char * ) "main_Compute2_1_in_5 = NULL;\n",
    ( char * ) "main_Compute2_1_out_1 = NULL;\n",
    ( char * ) "main_GetGlobal_1_in_1 = 0;\n",
    ( char * ) "main_GetGlobal_1_in_2 = NULL;\n",
    ( char * ) "main_GetGlobal_1_in_3 = NULL;\n",
    ( char * ) "main_GetGlobal_1_out_1 = NULL;\n",
    ( char * ) "main_GetGlobal_1_out_2 = NULL;\n",
    ( char * ) "main_Compute2_2_in_1 = \"loop+1\";\n",
    ( char * ) "main_Compute2_2_in_2 = \"loop\";\n",
    ( char * ) "main_Compute2_2_in_4 = NULL;\n",
    ( char * ) "main_Compute2_2_in_5 = NULL;\n",
    ( char * ) "main_Compute2_2_out_1 = NULL;\n",
    ( char * ) "main_Compute2_3_in_1 = \"global-local\";\n",
    ( char * ) "main_Compute2_3_in_2 = \"local\";\n",
    ( char * ) "main_Compute2_3_in_4 = \"global\";\n",
    ( char * ) "main_Compute2_3_out_1 = NULL;\n",
    ( char * ) "main_Compute_1_in_1 = \"[$0, $1]\";\n",
    ( char * ) "main_Compute_1_out_1 = NULL;\n",
    ( char * ) "_java_control = 0;\n",
    ( char * ) "main_SetGlobal_1_in_3 = NULL;\n",
    ( char * ) "main_SetLocal_1_in_3 = NULL;\n",
    ( char * ) "Executive(\"product version 4 2 0\");\n", ( char * ) "$sync\n",
    ( char * ) "// This network contains DXLink tools.  Therefore, the "
               "following line(s)\n",
    ( char * ) "// that would cause an execution when run in script mode have "
               "been \n",
    ( char * ) "// commented out.  This will facilitate the use of the DXLink "
               "routines\n",
    ( char * ) "// exDXLLoadScript() and DXLLoadVisualProgram() when the "
               "DXLink\n",
    ( char * ) "// application is connected to an executive.\n",
    ( char * ) "// main();\n", (char *)NULL


BEGIN				{
				    STATE = 0;
				}

$0 == "} YYSTYPE;"		{
				    STATE = 1;
				}

$0 == "#ifndef YYSTYPE"		{
				    if (STATE == 1)
				    {
					STATE = 2;
					next;
				    }
				}

$0 == "#define YYSTYPE int"	{
				    if (STATE == 2)
				    {
					STATE = 3;
					next;
				    }
				}

$0 == "#endif"			{
				    if (STATE == 3)
				    {
					STATE = 4;
					next;
				    }
				}

				{ print $0; }

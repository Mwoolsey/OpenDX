/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

void d2u(char *);  /* Dos -> Unix path conversion */
void u2d(char *);  /* Unix -> Dos path conversion */
void removeQuotes(char *); /* Remove quotes from string */
void addQuotes(char *); /* Add quotes around string 
							(make sure string has enough space already allocated.) */
int getenvstr(char *name, char *value); /* copy environment string name into value */
int putenvstr(char *name, char *value, int echo); /* set env str name with value */


/*
 * autocutsel.c by Michael Witrant <mike @ lepton . fr>
 * auto-update cutbuffer and selection when one of them change.
 * 
 * Most code taken from:
 * * clear-cut-buffers.c by "E. Jay Berkenbilt" <ejb @ ql . org>
 *   in this messages:
 *     http://boudicca.tux.org/mhonarc/ma-linux/2001-Feb/msg00824.html
 * 
 * * xcutsel.c by Ralph Swick, DEC/Project Athena
 *   from the XFree86 project: http://www.xfree86.org/
 * 
 * Copyright (c) 2001 Michael Witrant.
 * License: GPL (http://www.gnu.org/copyleft/gpl.html)
 * 
 */


#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/StdSel.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Shell.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Cardinals.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Widget box;
static Display* dpy;
static XtAppContext context;
static Atom selection;
static int buffer;

static XrmOptionDescRec optionDesc[] = {
     {"-selection", "selection", XrmoptionSepArg, NULL},
     {"-select",    "selection", XrmoptionSepArg, NULL},
     {"-sel",       "selection", XrmoptionSepArg, NULL},
     {"-s",         "selection", XrmoptionSepArg, NULL},
     {"-cutbuffer", "cutBuffer", XrmoptionSepArg, NULL},
     {"-cut",       "cutBuffer", XrmoptionSepArg, NULL},
     {"-c",         "cutBuffer", XrmoptionSepArg, NULL},
     {"-debug",     "debug",     XrmoptionNoArg,  "on"},
     {"-d",         "debug",     XrmoptionNoArg,  "on"},
     {"-pause",     "pause",     XrmoptionSepArg, NULL},
     {"-p",         "pause",     XrmoptionSepArg, NULL},
};

int Syntax(call)
  char *call;
{
    fprintf (stderr, "usage:  %s [-selection <name>] [-cutbuffer <number>] [-pause <delay>] [-debug]\n", 
             call);
    exit (1);
}

typedef struct {
   String  selection_name;
   int     buffer;
   String  debug_option;
   String  kill;
   int     pause;
   int     debug; 
   Atom    selection;
   char*   value;
   int     length;
   int     own_selection;
} OptionsRec;

OptionsRec options;

#define Offset(field) XtOffsetOf(OptionsRec, field)

static XtResource resources[] = {
     {"selection", "Selection", XtRString, sizeof(String),
       Offset(selection_name), XtRString, "PRIMARY"},
     {"cutBuffer", "CutBuffer", XtRInt, sizeof(int),
       Offset(buffer), XtRImmediate, (XtPointer)0},
     {"debug", "Debug", XtRString, sizeof(String),
       Offset(debug_option), XtRString, "off"},
     {"kill", "kill", XtRString, sizeof(String),
       Offset(kill), XtRString, "off"},
     {"pause", "Pause", XtRInt, sizeof(int),
       Offset(pause), XtRImmediate, (XtPointer)500},
};

#undef Offset

static Boolean ConvertSelection(w, selection, target,
                                type, value, length, format)
  Widget w;
  Atom *selection, *target, *type;
  XtPointer *value;
  unsigned long *length;
  int *format;
{
   Display* d = XtDisplay(w);
   XSelectionRequestEvent* req =
     XtGetSelectionRequest(w, *selection, (XtRequestId)NULL);
   
   if (*target == XA_TARGETS(d)) {
      Atom* targetP;
      Atom* std_targets;
      unsigned long std_length;
      XmuConvertStandardSelection(w, req->time, selection, target, type,
				  (XPointer*)&std_targets, &std_length, format);
      *value = XtMalloc(sizeof(Atom)*(std_length + 4));
      targetP = *(Atom**)value;
      *length = std_length + 4;
      *targetP++ = XA_STRING;
      *targetP++ = XA_TEXT(d);
      *targetP++ = XA_LENGTH(d);
      *targetP++ = XA_LIST_LENGTH(d);
      /*
       *targetP++ = XA_CHARACTER_POSITION(d);
       */
      memmove( (char*)targetP, (char*)std_targets, sizeof(Atom)*std_length);
      XtFree((char*)std_targets);
      *type = XA_ATOM;
      *format = 32;
      return True;
   }
   if (*target == XA_STRING || *target == XA_TEXT(d)) {
      *type = XA_STRING;
      *value = XtMalloc((Cardinal) options.length);
      memmove( (char *) *value, options.value, options.length);
      *length = options.length;
      *format = 8;
      return True;
   }
   if (*target == XA_LIST_LENGTH(d)) {
      long *temp = (long *) XtMalloc (sizeof(long));
      *temp = 1L;
      *value = (XtPointer) temp;
      *type = XA_INTEGER;
      *length = 1;
      *format = 32;
      return True;
   }
   if (*target == XA_LENGTH(d)) {
      long *temp = (long *) XtMalloc (sizeof(long));
      *temp = options.length;
      *value = (XtPointer) temp;
      *type = XA_INTEGER;
      *length = 1;
      *format = 32;
      return True;
   }
#ifdef notdef
   if (*target == XA_CHARACTER_POSITION(d)) {
      long *temp = (long *) XtMalloc (2 * sizeof(long));
      temp[0] = ctx->text.s.left + 1;
      temp[1] = ctx->text.s.right;
      *value = (XtPointer) temp;
      *type = XA_SPAN(d);
      *length = 2;
      *format = 32;
      return True;
   }
#endif /* notdef */
   if (XmuConvertStandardSelection(w, req->time, selection, target, type,
				   (XPointer *)value, length, format))
     return True;
   
   /* else */
   return False;
}


static void LoseSelection(w, selection)
  Widget w;
  Atom *selection;
{
   options.own_selection = 0;
}

static void StoreBuffer(w, client_data, selection, type, value, length, format)
  Widget w;
  XtPointer client_data;
  Atom *selection, *type;
  XtPointer value;
  unsigned long *length;
  int *format;
{
   if (*type == 0 || *type == XT_CONVERT_FAIL || *length == 0) {
      return;
   }
   
   if (!options.value || *length != options.length || memcmp(options.value, value, options.length))
     {
	options.value = value;
	options.length = strlen(options.value);
	if (options.debug)
	  {
	     printf("sel -> cut: ");
	     fwrite(options.value, options.length, 1, stdout);
	     printf("\n");
	  }
	
	XStoreBuffer( XtDisplay(w), (char*)options.value, (int)(options.length),
		     buffer );
     } else
     XtFree(value);
}


void timeout(XtPointer p, XtIntervalId* i)
{
   char *value;
   int length;

   value = XFetchBuffer(dpy, &length, buffer);
   if (!options.value || length != options.length || memcmp(options.value, value, options.length))
     {
	if (XtOwnSelection(box, selection,
			   0, //XtLastTimestampProcessed(dpy),
			   ConvertSelection, LoseSelection, NULL) == True)
	  {
	     options.own_selection = 1;
	     if (options.value) XFree(options.value);
	     options.value = value;
	     options.length = length;
	     if (options.debug)
	       {
		  
		  printf("cut -> sel: ");
		  fwrite(options.value, options.length, 1, stdout);
		  printf("\n");
	       }
	  } else
	  {
	     printf("unable to cut -> sel\n");
	  }
     } else
     {
	XFree(value);
	if (!options.own_selection)
	  XtGetSelectionValue(box, selection, XA_STRING,
			      StoreBuffer, NULL,
			      XtLastTimestampProcessed(XtDisplay(box)));
     }
   
   XtAppAddTimeOut(context, options.pause, timeout, 0);
}

int main(int argc, char* argv[])
{
   Widget top;
   top = XtVaAppInitialize(&context, "AutoCutSel",
			   optionDesc, XtNumber(optionDesc), &argc, argv, NULL,
			   XtNoverrideRedirect, True,
			   XtNgeometry, "-10-10",
			   0);

   if (argc != 1) Syntax(argv[0]);

   XtGetApplicationResources(top, (XtPointer)&options,
			     resources, XtNumber(resources),
			     NULL, ZERO );


   if (strcmp(options.debug_option, "on") == 0)
     options.debug = 1;
   else
     options.debug = 0;
   
   options.value = NULL;
   options.length = 0;

   options.own_selection = 0;
   
   box = XtCreateManagedWidget("box", boxWidgetClass, top, NULL, 0);
   dpy = XtDisplay(top);
   
   selection = XInternAtom(dpy, "PRIMARY", 0);
   buffer = 0;
   
   XtAppAddTimeOut(context, options.pause, timeout, 0);
   XtRealizeWidget(top);
   XtAppMainLoop(context);
   return 0;
}



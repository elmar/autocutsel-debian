/*
 * autocutsel.c by Michael Witrant <mike @ lepton . fr>
 * auto-update cutbuffer and selection when one of them change.
 * 
 * Code stolen from:
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Widget box;
static Display* dpy;
static XtAppContext context;
static Atom selection;
static int buffer;
static char *current = NULL;
static int current_length = 0;

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
      *value = XtMalloc((Cardinal) current_length);
      memmove( (char *) *value, current, current_length);
      *length = current_length;
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
      *temp = current_length;
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
#ifdef XKB
      XkbStdBell( XtDisplay(w), XtWindow(w), 0, XkbBI_MinorError );
#else
      XBell( XtDisplay(w), 0 );
#endif
      return;
   }
   
   if (!current || *length != current_length || memcmp(current, value, current_length))
     {
	current = value;
	current_length = strlen(current);
	printf("sel -> cut: ");
	fwrite(current, current_length, 1, stdout);
	printf("\n");
	
	XStoreBuffer( XtDisplay(w), (char*)current, (int)(current_length),
		     buffer );
     } else
     XtFree(value);
}


void timeout(XtPointer p, XtIntervalId* i)
{
   char *value;
   int length;

   value = XFetchBuffer(dpy, &length, buffer);
   if (!current || length != current_length || memcmp(current, value, current_length))
     {
	if (XtOwnSelection(box, selection,
			   0, //XtLastTimestampProcessed(dpy),
			   ConvertSelection, LoseSelection, NULL))
	  {
	     if (current) XFree(current);
	     current = value;
	     current_length = length;
	     printf("cut -> sel: ");
	     fwrite(current, current_length, 1, stdout);
	     printf("\n");
	  } else
	  {
	     printf("unable to cut -> sel\n");
	  }
     } else
     {
	XFree(value);
	XtGetSelectionValue(box, selection, XA_STRING,
			    StoreBuffer, NULL,
			    XtLastTimestampProcessed(XtDisplay(box)));
     }
   
   XtAppAddTimeOut(context, 1000, timeout, 0);
}

int main(int argc, char* argv[])
{
   Widget top;
   top = XtVaAppInitialize(&context, "AutoCutSel",
			   0, 0, &argc, argv, NULL,
			   XtNoverrideRedirect, True,
			   XtNgeometry, "-10-10",
			   0);
   box = XtCreateManagedWidget("box", boxWidgetClass, top, NULL, 0);
   dpy = XtDisplay(top);
   
   selection = XInternAtom(dpy, "PRIMARY", 0);
   buffer = 0;
   
   XtAppAddTimeOut(context, 500, timeout, 0);
   XtRealizeWidget(top);
   XtAppMainLoop(context);
   return 0;
}



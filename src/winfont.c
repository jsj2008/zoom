/*
 *  A Z-Machine
 *  Copyright (C) 2000 Andrew Hunter
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Fonts for Win32
 */

#include "../config.h"

#if WINDOW_SYSTEM == 2

#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "zmachine.h"
#include "windisplay.h"
#include "xfont.h"

struct xfont
{
  enum
  {
    WINFONT_INTERNAL,
    WINFONT_FONT3
  } type;
  union
  {
    struct
    {
      HFONT handle;
    } win;
  } data;
};

static int fore;
static int back;

void xfont_initialise(void)
{
}

void xfont_shutdown(void)
{
}

#define DEFAULT_FONT "'Fixed' 9"

static int recur = 0;
/*
 * Internal format for windows font names
 *
 * "face name" width properties
 *
 * Where properties can be one or more of:
 *   b - bold
 *   i - italic
 *   u - underline
 *   f - fixed
 */
xfont* xfont_load_font(char* font)
{
  char   fontcopy[256];
  char*  face_name;
  char*  face_width;
  char*  face_props;
  xfont* xf;

  LOGFONT defn;
  HFONT   hfont;
  int x;

  if (strcmp(font, "font3") == 0)
    {
      xf = malloc(sizeof(struct xfont));

      xf->type = WINFONT_FONT3;
      return xf;
    }
  
  if (recur > 2)
    {
      zmachine_fatal("Unable to load font, and unable to load default font " DEFAULT_FONT);
    }

  recur++;

  if (strlen(font) > 256)
    {
      zmachine_warning("Invalid font name (too long)");
      
      xf = xfont_load_font(DEFAULT_FONT);
      recur--;
      return xf;
    }

  /* Get the face name */
  strcpy(fontcopy, font);
  x = 0;
  while (fontcopy[x++] != '\'')
    {
      if (fontcopy[x] == 0)
	{
	  zmachine_warning("Invalid font name: %s (font name must be in single quotes)", font);

	  xf = xfont_load_font(DEFAULT_FONT);
	  recur--;
	  return xf;
	}
    }

  face_name = &fontcopy[x];

  x--;
  while (fontcopy[++x] != '\'')
    {
      if (fontcopy[x] == 0)
	{
	  zmachine_warning("Invalid font name: %s (missing \')", font);

	  xf = xfont_load_font(DEFAULT_FONT);
	  recur--;
	  return xf;
	}
    }
  fontcopy[x] = 0;

  /* Get the font width */
  while (fontcopy[++x] == ' ')
    {
      if (fontcopy[x] == 0)
	{
	  zmachine_warning("Invalid font name: %s (no font size specified)", font);

	  xf = xfont_load_font(DEFAULT_FONT);
	  recur--;
	  return xf;
	}
    }

  face_width = &fontcopy[x];

  while (fontcopy[x] >= '0' &&
	 fontcopy[x] <= '9')
    x++;

  if (fontcopy[x] != ' ' &&
      fontcopy[x] != 0)
    {
      zmachine_warning("Invalid font name: %s (invalid size)", font);

      xf = xfont_load_font(DEFAULT_FONT);
      recur--;
      return xf;
    }

  if (fontcopy[x] != 0)
    {
      fontcopy[x] = 0;
      face_props  = &fontcopy[x+1];
    }
  else
    face_props = NULL;

  defn.lfHeight         = -MulDiv(atoi(face_width),
				  GetDeviceCaps(mainwindc, LOGPIXELSY),
				  72);
  defn.lfWidth          = 0;
  defn.lfEscapement     = defn.lfOrientation = 0;
  defn.lfWeight         = 0;
  defn.lfItalic         = 0;
  defn.lfUnderline      = 0;
  defn.lfStrikeOut      = 0;
  defn.lfCharSet        = ANSI_CHARSET;
  defn.lfOutPrecision   = OUT_DEFAULT_PRECIS;
  defn.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
  defn.lfQuality        = DEFAULT_QUALITY;
  defn.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
  strcpy(defn.lfFaceName, face_name);

  if (face_props != NULL)
    {
      for (x=0; face_props[x] != 0; x++)
	{
	  switch (face_props[x])
	    {
	    case 'f':
	    case 'F':
	      defn.lfPitchAndFamily = FIXED_PITCH|FF_DONTCARE;
	      break;

	    case 'b':
	      defn.lfWeight = FW_BOLD;
	      break;

	    case 'B':
	      defn.lfWeight = FW_EXTRABOLD;
	      break;

	    case 'i':
	    case 'I':
	      defn.lfItalic = TRUE;
	      break;

	    case 'u':
	    case 'U':
	      defn.lfUnderline = TRUE;
	      break;
	    }
	}
    }

  hfont = CreateFontIndirect(&defn);

  if (hfont == NULL)
    {
      zmachine_warning("Couldn't load font %s", font);
      xf = xfont_load_font(DEFAULT_FONT);
    }
  else
    {
      xf = malloc(sizeof(struct xfont));
      xf->type = WINFONT_INTERNAL;
      xf->data.win.handle = hfont;
    }
  
  recur--;
  return xf;
}

void xfont_release_font(xfont* font)
{
  switch (font->type)
    {
    case WINFONT_INTERNAL:
      DeleteObject(font->data.win.handle);
      break;
      
    default:
      break;
    }
  free(font);
}

void xfont_set_colours(int foreground,
		       int background)
{
  fore = foreground%14;
  back = background%14;
}

int xfont_get_width(xfont* font)
{
  switch (font->type)
    {
    case WINFONT_INTERNAL:
      {
	HGDIOBJ ob;
	TEXTMETRIC metric;

	ob = SelectObject(mainwindc,
			  font->data.win.handle);
	
	GetTextMetrics(mainwindc, &metric);

	return metric.tmAveCharWidth;
      }
      
    case WINFONT_FONT3:
      break;
    }

  zmachine_fatal("Programmer is a spoon");
  return -1;
}

int xfont_get_height(xfont* font)
{
  switch (font->type)
    {
    case WINFONT_INTERNAL:
      {
	TEXTMETRIC metric;
	
	SelectObject(mainwindc,
		     font->data.win.handle);

	GetTextMetrics(mainwindc, &metric);

	return metric.tmHeight;
      }

    case WINFONT_FONT3:
      break;
    }
  
  zmachine_fatal("Programmer is a spoon");
  return -1;
}

int xfont_get_ascent(xfont* font)
{
  switch (font->type)
    {
    case WINFONT_INTERNAL:
      {
	TEXTMETRIC metric;
	
	SelectObject(mainwindc,
		     font->data.win.handle);

	GetTextMetrics(mainwindc, &metric);

	return metric.tmAscent;
      }

    case WINFONT_FONT3:
      break;
    }
  
  zmachine_fatal("Programmer is a spoon");
  return -1;
}

int xfont_get_descent(xfont* font)
{
  switch (font->type)
    {
    case WINFONT_INTERNAL:
      {
	TEXTMETRIC metric;
	
	SelectObject(mainwindc,
		     font->data.win.handle);

	GetTextMetrics(mainwindc, &metric);

	return metric.tmDescent;
      }

    case WINFONT_FONT3:
      return 0;
    }
  
  zmachine_fatal("Programmer is a spoon");
  return -1;
}

int xfont_get_text_width(xfont*     font,
			 const int* string,
			 int        len)
{
  switch (font->type)
    {
    case WINFONT_INTERNAL:
      {
	WCHAR* wide_str;
	int    x;
	SIZE   size;
	
	SelectObject(mainwindc,
		     font->data.win.handle);

	wide_str = malloc(sizeof(WCHAR)*(len+1));

	for (x=0; x<len; x++)
	  wide_str[x] = string[x];
	wide_str[len] = 0;
	
	GetTextExtentPoint32W(mainwindc,
			      wide_str,
			      len,
			      &size);

	free(wide_str);
	
	return size.cx;
      }

    case WINFONT_FONT3:
      return 0;
    }
  
  zmachine_fatal("Programmer is a spoon");
  return -1;  
}


void xfont_plot_string(xfont*     font,
		       HDC        dc,
		       int        xpos,
		       int        ypos,
		       const int* string,
		       int        len)
{
  switch (font->type)
    {
    case WINFONT_INTERNAL:
      {
	WCHAR* wide_str;
	int    x;
	SIZE   size;
	RECT   rct;
	
	SelectObject(dc,
		     font->data.win.handle);

	wide_str = malloc(sizeof(WCHAR)*(len+1));

	for (x=0; x<len; x++)
	  wide_str[x] = string[x];
	wide_str[len] = 0;
	
	GetTextExtentPoint32W(dc,
			      wide_str,
			      len,
			      &size);
	SetTextAlign(dc, TA_TOP|TA_LEFT);
	SetTextColor(dc, wincolour[fore]);
	SetBkColor(dc, wincolour[back]);

	rct.left   = xpos;
	rct.top    = ypos;
	rct.right  = xpos+size.cx;
	rct.bottom = ypos+xfont_get_height(font);

	/*
	DrawTextW(dc,
		  wide_str,
		  len,
		  &rct,
		  DT_SINGLELINE);
	*/
	ExtTextOutW(dc,
		    xpos, ypos,
		    ETO_OPAQUE, &rct,
		    wide_str, len,
		    NULL);

	free(wide_str);
      }
      break;

    case WINFONT_FONT3:

    default:
      zmachine_fatal("Programmer is a spoon");
    }
}

#endif

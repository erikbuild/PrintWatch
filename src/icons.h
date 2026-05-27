/* ABOUTME: Printer model icon lookup and drawing.
   ABOUTME: Maps model strings to cached ICON resource handles. */

#ifndef ICONS_H
#define ICONS_H

#include <Types.h>
#include <Quickdraw.h>

#define kGenericIconID 200

void Icons_Init(void);

void Icons_DrawForModel(const char *model, const Rect *iconRect);

#endif

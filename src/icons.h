/* ABOUTME: Printer model image lookup and drawing.
   ABOUTME: Maps model strings to cached PIMG resource handles. */

#ifndef ICONS_H
#define ICONS_H

void Icons_Init(void);

/* Get the pixel dimensions of the image for a model. Returns 1 if found, 0 if not. */
int Icons_GetSize(const char *model, int *width, int *height);

/* Draw the printer image at the given position. */
void Icons_Draw(const char *model, int x, int y);

#endif

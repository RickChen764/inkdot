#ifndef INKDOT_D2D_INIT_H
#define INKDOT_D2D_INIT_H

#include "app.h"

bool initD2D(App& app);
void applyTheme(App& app, int themeIndex);
void updateTextFormats(App& app);
void updateOverlayFormats(App& app);
void ensureThemePreviewFormats(App& app);
void createTypography(App& app);
bool createRenderTarget(App& app);

#endif // INKDOT_D2D_INIT_H

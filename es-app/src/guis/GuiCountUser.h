#pragma once
#ifndef ES_APP_GUIS_GUI_COUNT_USER_H
#define ES_APP_GUIS_GUI_COUNT_USER_H

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "FileData.h"
#include "Window.h"
#include "components/ButtonComponent.h"
class GuiCountUser : public GuiComponent
{
public:
    GuiCountUser(Window *window, int countCredits, int countPlayer, FileData *game, std::function<void(int)> launchGame);

private:
    MenuComponent mMenu;
    std::function<void(int)> mLaunchGameCallback;
};

#endif
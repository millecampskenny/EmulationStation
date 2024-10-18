#include "GuiCountUser.h"

GuiCountUser::GuiCountUser(Window *window, int countCredits, int countPlayer, FileData *game, std::function<void(int)> launchGame) : GuiComponent(window), mMenu(window, "Number of players"), mLaunchGameCallback(launchGame)
{

    int nbButtons = countCredits < countPlayer ? countCredits : countPlayer;
    for (int i = 0; i < nbButtons; i++)
    {
        ComponentListRow row;
        row.addElement(
            std::make_shared<ButtonComponent>(
                window, std::to_string(i + 1) + (i + 1 < 2 ? " Player" : " Players"), "", [i, this]()
                { mLaunchGameCallback(i + 1); delete this; }),
            true, true);
        mMenu.addRow(row, i == 0);
    }

    ComponentListRow rowCancel;
    rowCancel.addElement(std::make_shared<ButtonComponent>(window,
                                                           "Cancel", "", [this]()
                                                           {
                        delete this;
                        return true; }),
                         true, true);
    mMenu.addRow(rowCancel);

    addChild(&mMenu);

    setSize(mMenu.getSize());
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, Renderer::getScreenHeight() * 0.15f);
}
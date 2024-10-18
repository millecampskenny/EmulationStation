#include "GuiCountUser.h"

GuiCountUser::GuiCountUser(Window *window, int countCredits, int countPlayer, FileData *game, std::function<void(int)> launchGame) : GuiComponent(window), mMenu(window, "Number of players"), mLaunchGameCallback(launchGame)
{

    int nbButtons = countCredits > countPlayer ? countCredits : countPlayer;
    for (int i = 0; i < nbButtons; i++)
    {
        mMenu.addButton(std::to_string(i + 1) + (i + 1 < 2 ? " Joueur" : " Joueurs"), "", [i, this]()
                        { mLaunchGameCallback(i + 1); });
    }

    addChild(&mMenu);

    setSize(mMenu.getSize());
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, Renderer::getScreenHeight() * 0.15f);
}
#pragma once
#ifndef ES_APP_VIEWS_GAME_LIST_BASIC_GAME_LIST_VIEW_H
#define ES_APP_VIEWS_GAME_LIST_BASIC_GAME_LIST_VIEW_H

#include "components/TextListComponent.h"
#include "views/gamelist/ISimpleGameListView.h"

class BasicGameListView : public ISimpleGameListView
{
public:
	BasicGameListView(Window *window, FileData *root);

	// Called when a FileData* is added, has its metadata changed, or is removed
	virtual void onFileChanged(FileData *file, FileChangeType change) override;

	virtual void onThemeChanged(const std::shared_ptr<ThemeData> &theme) override;

	virtual FileData *getCursor() override;
	virtual void setCursor(FileData *file, bool refreshListCursorPos = false) override;
	virtual int getViewportTop() override;
	virtual void setViewportTop(int index) override;

	virtual const char *getName() const override { return "basic"; }

	virtual std::vector<HelpPrompt> getHelpPrompts() override;
	virtual void launch(FileData *game) override;

	void onFocusLost() override;

protected:
	virtual std::string getQuickSystemSelectRightButton() override;
	virtual std::string getQuickSystemSelectLeftButton() override;
	virtual void populateList(const std::vector<FileData *> &files) override;
	virtual void remove(FileData *game, bool deleteFile, bool refreshView = true) override;
	virtual void addPlaceholder();
 	virtual	void launchGame(int countCredits, FileData* game) override;

	TextListComponent<FileData *> mList;
};

#endif // ES_APP_VIEWS_GAME_LIST_BASIC_GAME_LIST_VIEW_H

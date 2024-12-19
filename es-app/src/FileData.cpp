#include "FileData.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "AudioManager.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "InputManager.h"
#include "Log.h"
#include "MameNames.h"
#include "platform.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include "Window.h"
#define Window XWindow
#define Font XFont
#include <X11/Xlib.h>
#undef Window
#undef Font
#include <assert.h>
#include <X11/extensions/XTest.h>
#include <thread>	// Pour std::thread
#include <chrono>	// Pour std::chrono::seconds
#include <unistd.h> // Pour usleep
#include <fstream>	// Pour std::ofstream
#include <iostream> // Pour std::cout et std::cerr

FileData::FileData(FileType type, const std::string &path, SystemEnvironmentData *envData, SystemData *system)
	: mType(type), mPath(path), mSystem(system), mEnvData(envData), mSourceFileData(NULL), mParent(NULL), metadata(type == GAME ? GAME_METADATA : FOLDER_METADATA) // metadata is REALLY set in the constructor!
{
	// metadata needs at least a name field (since that's what getName() will return)
	if (metadata.get("name").empty())
		metadata.set("name", getDisplayName());
	mSystemName = system->getName();
	metadata.resetChangedFlag();
}

FileData::~FileData()
{
	if (mParent)
		mParent->removeChild(this);

	if (mType == GAME)
		mSystem->getIndex()->removeFromIndex(this);

	mChildren.clear();
}

std::string FileData::getDisplayName() const
{
	std::string stem = Utils::FileSystem::getStem(mPath);
	if (mSystem && mSystem->hasPlatformId(PlatformIds::ARCADE) || mSystem->hasPlatformId(PlatformIds::NEOGEO))
		stem = MameNames::getInstance()->getRealName(stem);

	return stem;
}

std::string FileData::getCleanName() const
{
	return Utils::String::removeParenthesis(this->getDisplayName());
}

const std::string FileData::getThumbnailPath() const
{
	std::string thumbnail = metadata.get("thumbnail");

	// no thumbnail, try image
	if (thumbnail.empty())
	{
		thumbnail = metadata.get("image");

		// no image, try to use local image
		if (thumbnail.empty() && Settings::getInstance()->getBool("LocalArt"))
		{
			const char *extList[2] = {".png", ".jpg"};
			for (int i = 0; i < 2; i++)
			{
				if (thumbnail.empty())
				{
					std::string path = mEnvData->mStartPath + "/images/" + getDisplayName() + "-image" + extList[i];
					if (Utils::FileSystem::exists(path))
						thumbnail = path;
				}
			}
		}
	}

	return thumbnail;
}

const std::string &FileData::getName()
{
	return metadata.get("name");
}

const std::string &FileData::getSortName()
{
	if (metadata.get("sortname").empty())
		return metadata.get("name");
	else
		return metadata.get("sortname");
}

const std::vector<FileData *> &FileData::getChildrenListToDisplay()
{

	FileFilterIndex *idx = CollectionSystemManager::get()->getSystemToView(mSystem)->getIndex();
	if (idx->isFiltered())
	{
		mFilteredChildren.clear();
		for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
		{
			if (idx->showFile((*it)))
			{
				mFilteredChildren.push_back(*it);
			}
		}

		return mFilteredChildren;
	}
	else
	{
		return mChildren;
	}
}

const std::string FileData::getVideoPath() const
{
	std::string video = metadata.get("video");

	// no video, try to use local video
	if (video.empty() && Settings::getInstance()->getBool("LocalArt"))
	{
		std::string path = mEnvData->mStartPath + "/images/" + getDisplayName() + "-video.mp4";
		if (Utils::FileSystem::exists(path))
			video = path;
	}

	return video;
}

const std::string FileData::getMarqueePath() const
{
	std::string marquee = metadata.get("marquee");

	// no marquee, try to use local marquee
	if (marquee.empty() && Settings::getInstance()->getBool("LocalArt"))
	{
		const char *extList[2] = {".png", ".jpg"};
		for (int i = 0; i < 2; i++)
		{
			if (marquee.empty())
			{
				std::string path = mEnvData->mStartPath + "/images/" + getDisplayName() + "-marquee" + extList[i];
				if (Utils::FileSystem::exists(path))
					marquee = path;
			}
		}
	}

	return marquee;
}

const std::string FileData::getImagePath() const
{
	std::string image = metadata.get("image");

	// no image, try to use local image
	if (image.empty())
	{
		const char *extList[2] = {".png", ".jpg"};
		for (int i = 0; i < 2; i++)
		{
			if (image.empty())
			{
				std::string path = mEnvData->mStartPath + "/images/" + getDisplayName() + "-image" + extList[i];
				if (Utils::FileSystem::exists(path))
					image = path;
			}
		}
	}

	return image;
}

std::vector<FileData *> FileData::getFilesRecursive(unsigned int typeMask, bool displayedOnly) const
{
	std::vector<FileData *> out;
	FileFilterIndex *idx = mSystem->getIndex();

	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if ((*it)->getType() & typeMask)
		{
			if (!displayedOnly || !idx->isFiltered() || idx->showFile(*it))
				out.push_back(*it);
		}

		if ((*it)->getChildren().size() > 0)
		{
			std::vector<FileData *> subchildren = (*it)->getFilesRecursive(typeMask, displayedOnly);
			out.insert(out.cend(), subchildren.cbegin(), subchildren.cend());
		}
	}

	return out;
}

std::string FileData::getKey()
{
	return getFileName();
}

const bool FileData::isArcadeAsset()
{
	const std::string stem = Utils::FileSystem::getStem(mPath);
	return (
		(mSystem && (mSystem->hasPlatformId(PlatformIds::ARCADE) || mSystem->hasPlatformId(PlatformIds::NEOGEO))) &&
		(MameNames::getInstance()->isBios(stem) || MameNames::getInstance()->isDevice(stem)));
}

FileData *FileData::getSourceFileData()
{
	return this;
}

void FileData::addChild(FileData *file)
{
	assert(mType == FOLDER);
	assert(file->getParent() == NULL);

	const std::string key = file->getKey();
	if (mChildrenByFilename.find(key) == mChildrenByFilename.cend())
	{
		mChildrenByFilename[key] = file;
		mChildren.push_back(file);
		file->mParent = this;
	}
}

void FileData::removeChild(FileData *file)
{
	assert(mType == FOLDER);
	assert(file->getParent() == this);
	mChildrenByFilename.erase(file->getKey());
	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
	{
		if (*it == file)
		{
			file->mParent = NULL;
			mChildren.erase(it);
			return;
		}
	}

	// File somehow wasn't in our children.
	assert(false);
}

void FileData::sort(ComparisonFunction &comparator, bool ascending)
{
	if (ascending)
	{
		std::stable_sort(mChildren.begin(), mChildren.end(), comparator);
		for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++)
		{
			if ((*it)->getChildren().size() > 0)
				(*it)->sort(comparator, ascending);
		}
	}
	else
	{
		std::stable_sort(mChildren.rbegin(), mChildren.rend(), comparator);
		for (auto it = mChildren.rbegin(); it != mChildren.rend(); it++)
		{
			if ((*it)->getChildren().size() > 0)
				(*it)->sort(comparator, ascending);
		}
	}
}

void FileData::creditGame(int countCredit)
{
	// Attendre 30 secondes
	std::this_thread::sleep_for(std::chrono::seconds(30));

	// Ouvrir une connexion à l'affichage X
	Display *display = XOpenDisplay(NULL);
	if (!display)
	{
		LOG(LogError) << "Cannot open display!";
		return; // Gérer l'erreur d'une autre manière si nécessaire
	}

	// Obtenir le keycode pour la touche 'Escape'
	KeyCode escapeKeyCode = XKeysymToKeycode(display, XStringToKeysym("Escape"));
	if (escapeKeyCode == 0)
	{
		LOG(LogError) << "Keycode for Escape not found!";
		XCloseDisplay(display);
		return; // Gérer l'erreur d'une autre manière si nécessaire
	}

	// Simuler l'appui sur la touche 'Escape'
	XTestFakeKeyEvent(display, escapeKeyCode, True, 0);
	XFlush(display);

	// Simuler le relâchement de la touche 'Escape'
	XTestFakeKeyEvent(display, escapeKeyCode, False, 0);
	XFlush(display);

	// Fermer la connexion à l'affichage X
	XCloseDisplay(display);
}

void FileData::sort(const SortType &type)
{
	sort(*type.comparisonFunction, type.ascending);
	mSortDesc = type.description;
}

void FileData::writeInsertCoinConfig(int playerNumber, int coinStatus)
{
	// Chemin vers le fichier retroarch.cfg
	std::string filePath = "/opt/retropie/configs/all/retroarch.cfg";
	// std::string filePath = "~/.config/retroarch/retroarch.cfg";

	std::ifstream configFileIn(filePath);
	if (!configFileIn.is_open())
	{
		std::cerr << "Impossible d'ouvrir le fichier en lecture: " << filePath << std::endl;
		return;
	}

	// Lire tout le fichier dans une liste de chaînes de caractères
	std::vector<std::string> fileLines;
	std::string line;
	while (std::getline(configFileIn, line))
	{
		fileLines.push_back(line);
	}
	configFileIn.close();

	// Générer la clé et la valeur
	std::string key = "input_player" + std::to_string(playerNumber) + "_insert_coin";
	std::string value = (coinStatus > 0) ? "true" : "false";

	bool found = false;

	// Vérifier si la ligne existe déjà dans le fichier
	for (size_t i = 0; i < fileLines.size(); ++i)
	{
		if (fileLines[i].find(key) != std::string::npos)
		{
			// Modifier la ligne existante
			fileLines[i] = key + " = \"" + value + "\"";
			found = true;
			break;
		}
	}

	// Si la ligne n'existe pas, l'ajouter
	if (!found)
	{
		fileLines.push_back(key + " = \"" + value + "\"");
	}

	// Réécrire le fichier avec les nouvelles lignes
	std::ofstream configFileOut(filePath, std::ios::trunc);
	if (!configFileOut.is_open())
	{
		std::cerr << "Impossible d'ouvrir le fichier en écriture: " << filePath << std::endl;
		return;
	}

	for (const auto &fileLine : fileLines)
	{
		configFileOut << fileLine << "\n";
	}

	configFileOut.close();

	std::cout << "Configuration mise à jour : " << key << " = " << value << std::endl;
}

void FileData::launchGame(Window *window, int countCredit)
{
	LOG(LogInfo) << "Attempting to launch game...";

	for (int player = 1; player <= 4; ++player)
	{
		writeInsertCoinConfig(player, player <= countCredit ? 1 : 0);
	}

	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();
	InputManager::getInstance()->deinit();
	window->deinit();

	std::string command = mEnvData->mLaunchCommand;

	const std::string rom = Utils::FileSystem::getEscapedPath(getPath());
	const std::string basename = Utils::FileSystem::getStem(getPath());
	const std::string rom_raw = Utils::FileSystem::getPreferredPath(getPath());
	const std::string name = getName();

	command = Utils::String::replace(command, "%ROM%", rom);
	command = Utils::String::replace(command, "%BASENAME%", basename);
	command = Utils::String::replace(command, "%ROM_RAW%", rom_raw);

	Scripting::fireEvent("game-start", rom, basename, name);

	// Créer un nouveau thread pour exécuter creditGame
	// std::thread creditThread([this, countCredit]() {
	// 	creditGame(countCredit);
	// });

	LOG(LogInfo) << "	" << command;
	int exitCode = runSystemCommand(command);

	if (exitCode != 0)
	{
		LOG(LogWarning) << "...launch terminated with nonzero exit code " << exitCode << "!";
	}

	Scripting::fireEvent("game-end");

	window->init();
	InputManager::getInstance()->init();
	VolumeControl::getInstance()->init();
	window->normalizeNextUpdate();

	// update number of times the game has been launched

	FileData *gameToUpdate = getSourceFileData();

	int timesPlayed = gameToUpdate->metadata.getInt("playcount") + 1;
	gameToUpdate->metadata.set("playcount", std::to_string(static_cast<long long>(timesPlayed)));

	// update last played time
	gameToUpdate->metadata.set("lastplayed", Utils::Time::DateTime(Utils::Time::now()));
	CollectionSystemManager::get()->refreshCollectionSystems(gameToUpdate);

	gameToUpdate->mSystem->onMetaDataSavePoint();
}

CollectionFileData::CollectionFileData(FileData *file, SystemData *system)
	: FileData(file->getSourceFileData()->getType(), file->getSourceFileData()->getPath(), file->getSourceFileData()->getSystemEnvData(), system)
{
	// we use this constructor to create a clone of the filedata, and change its system
	mSourceFileData = file->getSourceFileData();
	refreshMetadata();
	mParent = NULL;
	metadata = mSourceFileData->metadata;
	mSystemName = mSourceFileData->getSystem()->getName();
}

CollectionFileData::~CollectionFileData()
{
	// need to remove collection file data at the collection object destructor
	if (mParent)
		mParent->removeChild(this);
	mParent = NULL;
}

std::string CollectionFileData::getKey()
{
	return getFullPath();
}

FileData *CollectionFileData::getSourceFileData()
{
	return mSourceFileData;
}

void CollectionFileData::refreshMetadata()
{
	metadata = mSourceFileData->metadata;
	mDirty = true;
}

const std::string &CollectionFileData::getName()
{
	if (mDirty)
	{
		mCollectionFileName = Utils::String::removeParenthesis(mSourceFileData->metadata.get("name"));
		mCollectionFileName += " [" + Utils::String::toUpper(mSourceFileData->getSystem()->getName()) + "]";
		mDirty = false;
	}

	if (Settings::getInstance()->getBool("CollectionShowSystemInfo"))
		return mCollectionFileName;
	return mSourceFileData->metadata.get("name");
}

// returns Sort Type based on a string description
FileData::SortType getSortTypeFromString(std::string desc)
{
	std::vector<FileData::SortType> SortTypes = FileSorts::SortTypes;
	// find it
	for (unsigned int i = 0; i < FileSorts::SortTypes.size(); i++)
	{
		const FileData::SortType &sort = FileSorts::SortTypes.at(i);
		if (sort.description == desc)
		{
			return sort;
		}
	}
	// if not found default to "name, ascending"
	return FileSorts::SortTypes.at(0);
}

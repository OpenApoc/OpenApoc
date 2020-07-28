#include <QApplication>
#include <QFileDialog>
#include <QProcess>
#include <QSize>
#include <array>
#include <utility>

#include "launcherwindow.h"
#include "ui_launcherwindow.h"

#include "framework/configfile.h"
#include "framework/filesystem.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/modinfo.h"
#include "framework/options.h"

using namespace OpenApoc;

static std::list<std::pair<UString, ModInfo>> enumerateMods()
{
	fs::path modPath = Options::modPath.get();
	if (!fs::is_directory(modPath))
	{
		LogError("Mod path \"%s\" not a valid directory", modPath.string());
		return {};
	}

	std::list<std::pair<UString, ModInfo>> foundMods;

	for (const auto &dentry : fs::directory_iterator(modPath))
	{
		// Skip any non-directories
		if (!fs::is_directory(dentry))
			continue;
		auto path = dentry.path();
		auto modInfo = ModInfo::getInfo(path.string());
		// Skip anything without a valid modinfo.xml
		if (!modInfo)
			continue;
		// Otherwise store the directory/modinfo pair
		foundMods.push_back({fs::relative(path, modPath).string(), *modInfo});
	}

	return foundMods;
}

constexpr std::array<QSize, 4> default_resolutions = {

    // Use {0,0} as a placeholder for 'custom', expected to be the first index
    QSize{0, 0}, QSize{640, 480}, QSize{1280, 720}, QSize{1920, 1080}};

constexpr QSize MINIMUM_RESOLUTION = {640, 480};
constexpr QSize MAXIMUM_RESOLUTION = {100000, 100000};

LauncherWindow::LauncherWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::LauncherWindow)
{
	const char *fake_argv = "OpenApoc";

	config().parseOptions(1, &fake_argv);

	this->currentFramework = mkup<OpenApoc::Framework>("OpenApoc_Launcher", false);
	ui->setupUi(this);

	connect(ui->exitButton, &QPushButton::clicked, this, &LauncherWindow::exit);
	connect(ui->playButton, &QPushButton::clicked, this, &LauncherWindow::play);
	connect(ui->browseCDFile, &QPushButton::clicked, this, &LauncherWindow::browseCDFile);
	connect(ui->browseCDDir, &QPushButton::clicked, this, &LauncherWindow::browseCDDir);
	connect(ui->browseDataDir, &QPushButton::clicked, this, &LauncherWindow::browseDataDir);
	connect(ui->resolutionBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this,
	        &LauncherWindow::setResolutionSelection);

	connect(ui->enabledModsList, &QListWidget::currentTextChanged, this,
	        &LauncherWindow::enabledModSelected);
	connect(ui->disabledModsList, &QListWidget::currentTextChanged, this,
	        &LauncherWindow::disabledModSelected);
	connect(ui->enableButton, &QPushButton::clicked, this, &LauncherWindow::enableModClicked);
	connect(ui->disableButton, &QPushButton::clicked, this, &LauncherWindow::disableModClicked);

	ui->customResolutionX->setValidator(
	    new QIntValidator(MINIMUM_RESOLUTION.width(), MAXIMUM_RESOLUTION.width(), this));
	ui->customResolutionY->setValidator(
	    new QIntValidator(MINIMUM_RESOLUTION.height(), MAXIMUM_RESOLUTION.height(), this));

	ui->fullscreenCheckBox->setCheckState(OpenApoc::Options::screenFullscreenOption.get()
	                                          ? Qt::CheckState::Checked
	                                          : Qt::CheckState::Unchecked);
	setupResolutionDisplay();

	ui->cdPath->setText(QString::fromStdString(OpenApoc::Options::cdPathOption.get()));
	ui->dataPath->setText(QString::fromStdString(OpenApoc::Options::dataPathOption.get()));

	setupModList();
}

LauncherWindow::~LauncherWindow() {}

void LauncherWindow::setupResolutionDisplay()
{
	auto &comboBox = *ui->resolutionBox;
	const QSize current_size = {OpenApoc::Options::screenWidthOption.get(),
	                            OpenApoc::Options::screenHeightOption.get()};
	bool is_custom = true;

	comboBox.clear();
	for (const auto &size : default_resolutions)
	{
		QString text;
		if (size == QSize{0, 0})
		{
			text = "Custom";
		}
		else
		{
			text = QString::number(size.width()) + " x " + QString::number(size.height());
		}
		comboBox.addItem(text, size);
		if (current_size == size)
		{
			comboBox.setCurrentIndex(comboBox.count() - 1);
			is_custom = false;
		}
	}

	auto &widthBox = *ui->customResolutionX;
	auto &heightBox = *ui->customResolutionY;

	widthBox.setText(QString::number(current_size.width()));
	heightBox.setText(QString::number(current_size.height()));

	if (is_custom)
	{
		widthBox.setEnabled(true);
		heightBox.setEnabled(true);
	}
	else
	{
		widthBox.setEnabled(false);
		heightBox.setEnabled(false);
	}
}

void LauncherWindow::setResolutionSelection(int index)
{
	const auto &comboBox = *ui->resolutionBox;
	const auto &selectionVariant = comboBox.currentData();
	const auto &size = selectionVariant.toSize();
	auto &widthBox = *ui->customResolutionX;
	auto &heightBox = *ui->customResolutionY;
	int x = 0;
	int y = 0;
	bool is_custom;

	if (size.height() == 0 || size.width() == 0)
	{
		x = widthBox.text().toInt();
		y = heightBox.text().toInt();
		is_custom = true;
	}
	else
	{
		x = size.width();
		y = size.height();
		is_custom = false;
	}
	OpenApoc::Options::screenWidthOption.set(x);
	OpenApoc::Options::screenHeightOption.set(y);

	if (is_custom)
	{
		widthBox.setEnabled(true);
		heightBox.setEnabled(true);
	}
	else
	{
		widthBox.setEnabled(false);
		heightBox.setEnabled(false);
	}
}

void LauncherWindow::saveConfig()
{
	const auto &comboBox = *ui->resolutionBox;
	// Index 0 is always custom resolution
	if (comboBox.currentIndex() == 0)
	{
		auto &widthBox = *ui->customResolutionX;
		auto &heightBox = *ui->customResolutionY;
		const auto x = widthBox.text().toInt();
		const auto y = heightBox.text().toInt();
		OpenApoc::Options::screenWidthOption.set(x);
		OpenApoc::Options::screenHeightOption.set(y);
	}
	else
	{
		const auto &selectionVariant = comboBox.currentData();
		const auto &size = selectionVariant.toSize();
		OpenApoc::Options::screenWidthOption.set(size.width());
		OpenApoc::Options::screenHeightOption.set(size.height());
	}

	OpenApoc::Options::screenFullscreenOption.set(ui->fullscreenCheckBox->checkState() ==
	                                              Qt::CheckState::Checked);
	OpenApoc::Options::cdPathOption.set(ui->cdPath->text().toStdString());
	OpenApoc::Options::dataPathOption.set(ui->dataPath->text().toStdString());
	OpenApoc::config().save();
	this->rebuildModList();
}

void LauncherWindow::play()
{
	saveConfig();
	this->currentFramework.reset();

#ifdef _WIN32
	QString path = "OpenApoc.exe";
#else
	QString path = QCoreApplication::applicationDirPath() + "/OpenApoc";
#endif

	LogWarning("Running \"%s\"", path.toStdString());
	const auto ret = QProcess::startDetached(path);
	if (!ret)
	{
		LogError("Failed to start OpenApoc process");
	}
	this->exit();
}

void LauncherWindow::browseCDFile()
{
	auto newPath = QFileDialog::getOpenFileName(this, "Select CD ISO/CUE file", ui->cdPath->text(),
	                                            "ISO files (*.iso);; CUE files (*.cue)");
	if (newPath.isNull())
	{
		// cancelled
		return;
	}

	ui->cdPath->setText(newPath);
}

void LauncherWindow::browseCDDir()
{
	auto newPath = QFileDialog::getExistingDirectory(this, "Select extracted/mounted CD directory",
	                                                 ui->cdPath->text());
	if (newPath.isNull())
	{
		// cancelled
		return;
	}

	ui->cdPath->setText(newPath);
}

void LauncherWindow::browseDataDir()
{
	auto newPath =
	    QFileDialog::getExistingDirectory(this, "Select data directory", ui->dataPath->text());
	if (newPath.isNull())
	{
		// cancelled
		return;
	}

	ui->dataPath->setText(newPath);
}

void LauncherWindow::exit()
{
	this->saveConfig();
	this->currentFramework.reset();
	QCoreApplication::quit();
}

void LauncherWindow::setupModList()
{
	auto foundMods = enumerateMods();
	auto enabledMods = split(Options::modList.get(), ":");

	ui->enabledModsList->clear();
	ui->disabledModsList->clear();

	// First set enabled mods in order

	for (const auto &enabledModName : enabledMods)
	{
		for (const auto &[modDir, modInfo] : foundMods)
		{
			if (modDir == enabledModName)
			{
				const auto &modName = modInfo.getName();
				ui->enabledModsList->addItem(QString::fromStdString(modName));
			}
		}
	}

	// Then fill the disabled modlist with anything else

	for (const auto &[modDir, modInfo] : foundMods)
	{
		bool enabled = false;
		// Is this enabled?
		for (const auto &enabledModName : enabledMods)
		{
			if (modDir == enabledModName)
			{
				enabled = true;
				break;
			}
		}
		const auto &modName = modInfo.getName();
		if (!enabled)
			ui->disabledModsList->addItem(QString::fromStdString(modName));
	}
}

void LauncherWindow::enabledModSelected(const QString &itemName)
{
	UString modName = itemName.toStdString();
	const auto foundMods = enumerateMods();
	for (const auto &mod : foundMods)
	{
		const auto &modInfo = mod.second;
		if (modInfo.getName() == modName)
		{
			this->showModInfo(modInfo);
			this->selectedModName = modName;
			return;
		}
	}
}

void LauncherWindow::disabledModSelected(const QString &itemName)
{
	UString modName = itemName.toStdString();
	const auto foundMods = enumerateMods();
	for (const auto &mod : foundMods)
	{
		const auto &modInfo = mod.second;
		if (modInfo.getName() == modName)
		{
			this->showModInfo(modInfo);
			this->selectedModName = modName;
			return;
		}
	}
}

void LauncherWindow::showModInfo(const ModInfo &info)
{
	ui->modName->setText(QString::fromStdString(info.getName()));
	ui->modAuthor->setText(QString::fromStdString(info.getAuthor()));
	ui->modVersion->setText(QString::fromStdString(info.getVersion()));
	ui->modDescription->setText(QString::fromStdString(info.getDescription()));

	auto linkText = format("<a href=\"%s\">%s</a>", info.getLink(), info.getLink());

	ui->modLink->setText(QString::fromStdString(linkText));
}

void LauncherWindow::rebuildModList()
{

	const auto foundMods = enumerateMods();

	std::list<UString> enabledMods;

	for (int i = 0; i < ui->enabledModsList->count(); i++)
	{
		auto listItem = ui->enabledModsList->item(i);
		LogAssert(listItem != nullptr);
		auto name = listItem->text().toStdString();

		for (const auto &[modDir, modInfo] : foundMods)
		{
			if (modInfo.getName() == name)
			{
				enabledMods.push_back(modDir);
				break;
			}
		}
	}

	UString modString;
	for (const auto &modDir : enabledMods)
	{
		if (modString != "")
			modString += ":";
		modString += modDir;
	}

	Options::modList.set(modString);
}

void LauncherWindow::enableModClicked()
{
	if (selectedModName == "")
		return;

	for (const auto &matchingItem :
	     ui->enabledModsList->findItems(QString::fromStdString(selectedModName), Qt::MatchExactly))
	{
		auto matchingName = matchingItem->text();
		// Already enabled
		if (matchingName.toStdString() == selectedModName)
		{
			return;
		}
	}

	ui->enabledModsList->addItem(QString::fromStdString(selectedModName));

	for (const auto &matchingItem :
	     ui->disabledModsList->findItems(QString::fromStdString(selectedModName), Qt::MatchExactly))
	{
		delete matchingItem;
	}

	this->rebuildModList();
}

void LauncherWindow::disableModClicked()
{
	if (selectedModName == "")
		return;

	for (const auto &matchingItem :
	     ui->disabledModsList->findItems(QString::fromStdString(selectedModName), Qt::MatchExactly))
	{
		auto matchingName = matchingItem->text();
		// Already disabled
		if (matchingName.toStdString() == selectedModName)
		{
			return;
		}
	}

	ui->disabledModsList->addItem(QString::fromStdString(selectedModName));

	for (const auto &matchingItem :
	     ui->enabledModsList->findItems(QString::fromStdString(selectedModName), Qt::MatchExactly))
	{
		delete matchingItem;
	}

	this->rebuildModList();
}

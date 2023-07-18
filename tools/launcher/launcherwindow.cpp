#include <QApplication>
#include <QFileDialog>
#include <QProcess>
#include <QSize>
#include <array>
#include <string_view>
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

constexpr std::array<QSize, 6> default_resolutions = {

    // Use {0,0} as a placeholder for 'custom', expected to be the first index
    QSize{0, 0},       QSize{640, 480},   QSize{1280, 720},
    QSize{1920, 1080}, QSize{2560, 1440}, QSize{3200, 1800}};

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

	connect(ui->languageBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this,
	        &LauncherWindow::setLanguageSelection);

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

	setupResolutionDisplay();
	setupScaling();
	setupScreenModes();
	setupDisplayNum();

	ui->cdPath->setText(QString::fromStdString(OpenApoc::Options::cdPathOption.get()));
	ui->dataPath->setText(QString::fromStdString(OpenApoc::Options::dataPathOption.get()));

	setupModList();
	updateAvailableLanguages();
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

void LauncherWindow::setupScreenModes()
{
	constexpr std::array<std::string_view, 3> screen_modes = {"windowed", "fullscreen",
	                                                          "borderless"};

	auto &comboBox = *ui->screenModeBox;
	comboBox.clear();
	int index = 0;
	for (const auto &option : screen_modes)
	{
		comboBox.addItem(option.data());
		if (option == Options::screenModeOption.get())
		{
			comboBox.setCurrentIndex(index);
		}
		++index;
	}
};

void LauncherWindow::setupDisplayNum()
{
	const auto displays = QGuiApplication::screens();
	auto &comboBox = *ui->displayNumBox;
	comboBox.clear();

	for (int i = 0; i < displays.count(); ++i)
	{
		comboBox.addItem(QString("Display #%1").arg(i));
	}

	int curDisplayValue = Options::screenDisplayNumberOption.get();
	if (curDisplayValue < displays.count())
	{
		comboBox.setCurrentIndex(curDisplayValue);
	}
}

enum class ScalingType
{
	Auto = 0,
	None,
	Scale_150,
	Scale_200,
	Scale_300,
	Scale_400,
	Custom, // Fallback value: if combination of options does not match any predefined option, just
	        // show 'custom' which, until changed to something else, won't modify stored options.
	_count
};

struct ScalingOption
{
	const ScalingType type;
	const std::string_view label;
	const int scale_value; // Corresponding screen scale values from options

	constexpr ScalingOption(ScalingType type, std::string_view label, int scale_value)
	    : type(type), label(label), scale_value(scale_value)
	{
	}
};

constexpr std::array<ScalingOption, static_cast<int>(ScalingType::_count)> scaling_options = {
    ScalingOption{ScalingType::Auto, "Auto", -1},
    ScalingOption{ScalingType::None, "None", 100},
    ScalingOption{ScalingType::Scale_150, "150%", 66},
    ScalingOption{ScalingType::Scale_200, "200%", 50},
    ScalingOption{ScalingType::Scale_300, "300%", 33},
    ScalingOption{ScalingType::Scale_400, "400%", 25},
    ScalingOption{ScalingType::Custom, "Custom", -1}};

void LauncherWindow::setupScaling()
{
	auto &comboBox = *ui->scaleBox;
	comboBox.clear();
	for (const auto &option : scaling_options)
	{
		comboBox.addItem(option.label.data());
	}

	const bool autoScale = OpenApoc::Options::screenAutoScale.get();
	ScalingType currentType = ScalingType::Custom;
	if (autoScale)
	{
		currentType = ScalingType::Auto;
	}
	else
	{
		const QSize screenScale = {OpenApoc::Options::screenScaleXOption.get(),
		                           OpenApoc::Options::screenScaleYOption.get()};

		for (const auto &option : scaling_options)
		{
			if (option.scale_value == screenScale.width() &&
			    option.scale_value == screenScale.height())
			{
				currentType = option.type;
				break;
			}
		}
	}

	comboBox.setCurrentIndex(static_cast<int>(currentType));
}

void LauncherWindow::saveScalingOptions()
{
	int index = ui->scaleBox->currentIndex();
	LogAssert(index >= 0 && index < scaling_options.size());

	if (scaling_options[index].type == ScalingType::Auto)
	{
		OpenApoc::Options::screenAutoScale.set(true);
	}
	else if (scaling_options[index].type == ScalingType::Custom)
	{
		// Do nothing - keep previous settings
	}
	else
	{
		OpenApoc::Options::screenAutoScale.set(false);
		OpenApoc::Options::screenScaleXOption.set(scaling_options[index].scale_value);
		OpenApoc::Options::screenScaleYOption.set(scaling_options[index].scale_value);
	}
}

void LauncherWindow::setLanguageSelection(int index)
{
	selectedLanguageID = ui->languageBox->itemData(index).toString().toStdString();
}

void LauncherWindow::saveConfig()
{
	saveScalingOptions();

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

	OpenApoc::Options::screenModeOption.set(ui->screenModeBox->currentText().toStdString());
	OpenApoc::Options::screenDisplayNumberOption.set(ui->displayNumBox->currentIndex());
	OpenApoc::Options::cdPathOption.set(ui->cdPath->text().toStdString());
	OpenApoc::Options::dataPathOption.set(ui->dataPath->text().toStdString());
	OpenApoc::Options::languageOption.set(selectedLanguageID);
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
	const auto ret = QProcess::startDetached(path, {});
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

	updateAvailableLanguages();
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
	updateAvailableLanguages();
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
	updateAvailableLanguages();
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

static QString getLanguageName(const std::string id)
{
	QLocale locale(QString::fromStdString(id));
	return locale.nativeLanguageName();
}

void LauncherWindow::updateAvailableLanguages()
{
	if (selectedLanguageID.empty())
	{
		selectedLanguageID = Options::languageOption.get();
		if (selectedLanguageID.empty())
		{
			// Just default to american english if not set
			selectedLanguageID = "en.UTF-8";
		}
	}

	ui->languageBox->clear();

	// FIXME: Currently just returns all supported languages of the first mod

	auto foundMods = enumerateMods();
	auto enabledMods = split(Options::modList.get(), ":");

	for (const auto &enabledModName : enabledMods)
	{
		for (const auto &[modDir, modInfo] : foundMods)
		{
			if (modDir == enabledModName)
			{
				for (const auto &languageID : modInfo.getSupportedLanguages())
				{
					ui->languageBox->addItem(getLanguageName(languageID),
					                         QString::fromStdString(languageID));
					if (selectedLanguageID == languageID)
					{
						ui->languageBox->setCurrentIndex(ui->languageBox->count() - 1);
					}
				}
			}
		}
	}
}

#include <QApplication>
#include <QFileDialog>
#include <QProcess>
#include <QSize>
#include <QtGlobal>
#include <array>
#include <utility>

#include "launcherwindow.h"
#include "ui_launcherwindow.h"

#include "framework/configfile.h"
#include "framework/framework.h"
#include "framework/logger.h"
#include "framework/shared_config.h"

using namespace OpenApoc;

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
	connect(ui->resolutionBox, QOverload<int>::of(&QComboBox::activated), this,
	        &LauncherWindow::setResolutionSelection);

	ui->customResolutionX->setValidator(
	    new QIntValidator(MINIMUM_RESOLUTION.width(), MAXIMUM_RESOLUTION.width(), this));
	ui->customResolutionY->setValidator(
	    new QIntValidator(MINIMUM_RESOLUTION.height(), MAXIMUM_RESOLUTION.height(), this));

	ui->fullscreenCheckBox->setCheckState(OpenApoc::Config::screenFullscreenOption.get()
	                                          ? Qt::CheckState::Checked
	                                          : Qt::CheckState::Unchecked);
	setupResolutionDisplay();

	ui->cdPath->setText(QString::fromStdString(OpenApoc::Config::cdPathOption.get().str()));
	ui->dataPath->setText(QString::fromStdString(OpenApoc::Config::dataPathOption.get().str()));
}

LauncherWindow::~LauncherWindow() {}

void LauncherWindow::setupResolutionDisplay()
{
	auto &comboBox = *ui->resolutionBox;
	const QSize current_size = {OpenApoc::Config::screenWidthOption.get(),
	                            OpenApoc::Config::screenHeightOption.get()};
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
	OpenApoc::Config::screenWidthOption.set(x);
	OpenApoc::Config::screenHeightOption.set(y);

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
		OpenApoc::Config::screenWidthOption.set(x);
		OpenApoc::Config::screenHeightOption.set(y);
	}
	else
	{
		const auto &selectionVariant = comboBox.currentData();
		const auto &size = selectionVariant.toSize();
		OpenApoc::Config::screenWidthOption.set(size.width());
		OpenApoc::Config::screenHeightOption.set(size.height());
	}

	OpenApoc::Config::screenFullscreenOption.set(ui->fullscreenCheckBox->checkState() ==
	                                             Qt::CheckState::Checked);
	OpenApoc::Config::cdPathOption.set(ui->cdPath->text().toStdString());
	OpenApoc::Config::dataPathOption.set(ui->dataPath->text().toStdString());
	OpenApoc::config().save();
}

void LauncherWindow::play()
{
	saveConfig();
	this->currentFramework.reset();

	auto path = QCoreApplication::applicationDirPath();
#ifdef _WIN32
	path += "/OpenApoc.exe";
#else
	path += "/OpenApoc";
#endif

	auto ret = QProcess::startDetached(path);
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
	this->currentFramework.reset();
	QCoreApplication::quit();
}

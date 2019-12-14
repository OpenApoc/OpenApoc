#include "library/strings.h"
#include <QMainWindow>
#include <map>
#include <memory>
#include <string>

namespace Ui
{
class LauncherWindow;
}

namespace OpenApoc
{
class Framework;
class ModInfo;
} // namespace OpenApoc

class LauncherWindow : public QMainWindow
{
	Q_OBJECT

  public:
	explicit LauncherWindow(QWidget *parent = 0);
	~LauncherWindow();

  private slots:
	void exit();
	void play();
	void setResolutionSelection(int index);

	void browseCDFile();
	void browseCDDir();
	void browseDataDir();

	void enabledModSelected(const QString &modName);
	void disabledModSelected(const QString &modName);
	void enableModClicked();
	void disableModClicked();

  private:
	void setupResolutionDisplay();
	void saveConfig();
	void setupModList();
	void showModInfo(const OpenApoc::ModInfo &info);
	void rebuildModList();

	std::unique_ptr<OpenApoc::Framework> currentFramework;
	std::unique_ptr<Ui::LauncherWindow> ui;

	OpenApoc::UString selectedModName;
};

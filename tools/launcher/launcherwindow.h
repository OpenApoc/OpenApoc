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

  private:
	void setupResolutionDisplay();
	void saveConfig();

	std::unique_ptr<OpenApoc::Framework> currentFramework;
	std::unique_ptr<Ui::LauncherWindow> ui;
};

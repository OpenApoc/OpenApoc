#include "cddetect.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QStorageInfo>
#include <QTextStream>

namespace OpenApoc::CDDetect
{

// Returns true if `dir` (non-recursively) contains a file named `name`, matched
// case-insensitively.
static bool caseInsensitiveFileExists(const QDir &dir, const QString &name)
{
	for (const auto &entry : dir.entryList(QDir::Files))
	{
		if (entry.compare(name, Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	return false;
}

bool isPlausibleCDPath(const QString &path)
{
	if (path.isEmpty())
	{
		return false;
	}

	const QFileInfo info(path);

	if (info.isFile())
	{
		const QString suffix = info.suffix();
		if (suffix.compare("iso", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
		if (suffix.compare("cue", Qt::CaseInsensitive) == 0)
		{
			const QDir cueDir = info.dir();
			QFile cueFile(path);
			if (cueFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				const QString text = QTextStream(&cueFile).readAll();
				static const QRegularExpression binRegex(
				    QStringLiteral("FILE\\s+\"?([^\"\\r\\n]+\\.bin)\"?"),
				    QRegularExpression::CaseInsensitiveOption);
				const auto match = binRegex.match(text);
				if (match.hasMatch())
				{
					const QString binName = QFileInfo(match.captured(1)).fileName();
					if (caseInsensitiveFileExists(cueDir, binName))
					{
						return true;
					}
				}
			}
			// Fall back to the conventional GOG naming if the FILE directive didn't yield a
			// match (or the referenced .bin doesn't exist alongside the .cue).
			return caseInsensitiveFileExists(cueDir, QStringLiteral("XCOM.BIN"));
		}
		return false;
	}

	if (info.isDir())
	{
		return caseInsensitiveFileExists(QDir(path), QStringLiteral("music"));
	}

	return false;
}

// Appends `candidate` to `list` unless an equivalent entry (case-insensitive) is already
// present.
static void addUniqueCandidate(QStringList &list, const QString &candidate)
{
	for (const auto &existing : list)
	{
		if (existing.compare(candidate, Qt::CaseInsensitive) == 0)
		{
			return;
		}
	}
	list.append(candidate);
}

static QStringList steamRoots()
{
	QStringList roots;
#ifdef _WIN32
	const QString installPath =
	    QSettings("HKEY_CURRENT_USER\\Software\\Valve\\Steam", QSettings::NativeFormat)
	        .value("InstallPath")
	        .toString();
	if (!installPath.isEmpty() && QDir(installPath).exists())
	{
		roots.append(installPath);
	}
#else
	const QString home = QDir::homePath();
	const QStringList candidates = {
	    home + "/.steam/steam",
	    home + "/.local/share/Steam",
	    home + "/.var/app/com.valvesoftware.Steam/.local/share/Steam",
	};
	for (const auto &candidate : candidates)
	{
		if (QDir(candidate).exists())
		{
			roots.append(candidate);
		}
	}
#endif
	return roots;
}

// Extracts all values of `"key"  "value"` pairs from Valve's VDF/ACF text format.
static QStringList extractQuotedValues(const QString &text, const QString &key)
{
	const QRegularExpression regex(
	    QStringLiteral("\"%1\"\\s*\"([^\"]*)\"").arg(QRegularExpression::escape(key)));
	QStringList values;
	auto it = regex.globalMatch(text);
	while (it.hasNext())
	{
		values.append(it.next().captured(1));
	}
	return values;
}

static QString readFile(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return {};
	}
	return QTextStream(&file).readAll();
}

static QStringList steamCandidates()
{
	QStringList candidates;

	for (const auto &root : steamRoots())
	{
		QStringList libraries = {root};

		const QString vdfPath = root + "/steamapps/libraryfolders.vdf";
		if (QFileInfo::exists(vdfPath))
		{
			const QString vdfText = readFile(vdfPath);
			for (const auto &path : extractQuotedValues(vdfText, "path"))
			{
				addUniqueCandidate(libraries, path);
			}
		}

		for (const auto &library : libraries)
		{
			const QString manifestPath = library + "/steamapps/appmanifest_7660.acf";
			if (!QFileInfo::exists(manifestPath))
			{
				continue;
			}
			const QString manifestText = readFile(manifestPath);
			const QStringList installDirs = extractQuotedValues(manifestText, "installdir");
			if (installDirs.isEmpty())
			{
				continue;
			}
			const QString gameDir = library + "/steamapps/common/" + installDirs.first();
			const QString candidate = gameDir + "/cd.iso";
			if (isPlausibleCDPath(candidate))
			{
				addUniqueCandidate(candidates, candidate);
			}
		}
	}

	return candidates;
}

static QStringList gogCandidates()
{
	QStringList candidates;
#ifdef _WIN32
	const QStringList registryKeys = {
	    "HKEY_LOCAL_MACHINE\\Software\\GOG.com\\Games\\1445249430",
	    "HKEY_LOCAL_MACHINE\\Software\\WOW6432Node\\GOG.com\\Games\\1445249430",
	};
	for (const auto &key : registryKeys)
	{
		const QString installPath =
		    QSettings(key, QSettings::NativeFormat).value("PATH").toString();
		if (installPath.isEmpty())
		{
			continue;
		}
		const QString candidate = installPath + "/CD/XCOM.cue";
		if (isPlausibleCDPath(candidate))
		{
			addUniqueCandidate(candidates, candidate);
		}
	}
#endif
	return candidates;
}

static QStringList mountedVolumeCandidates()
{
	QStringList candidates;
	for (const auto &volume : QStorageInfo::mountedVolumes())
	{
		if (!volume.isValid() || !volume.isReady())
		{
			continue;
		}
		const QString rootPath = volume.rootPath();
		if (isPlausibleCDPath(rootPath))
		{
			addUniqueCandidate(candidates, rootPath);
		}
	}
	return candidates;
}

QStringList detectCandidates()
{
	QStringList candidates;

	if (isPlausibleCDPath("./data/cd.iso"))
	{
		addUniqueCandidate(candidates, "./data/cd.iso");
	}
	for (const auto &candidate : steamCandidates())
	{
		addUniqueCandidate(candidates, candidate);
	}
	for (const auto &candidate : gogCandidates())
	{
		addUniqueCandidate(candidates, candidate);
	}
	for (const auto &candidate : mountedVolumeCandidates())
	{
		addUniqueCandidate(candidates, candidate);
	}

	return candidates;
}

} // namespace OpenApoc::CDDetect

#include "v2raycore.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "configurator.h"
#include "constants.h"
#include "utility.h"

V2RayCore::V2RayCore() {
  QString v2RayInstallFolderPath = Configurator::getV2RayInstallDirPath();
#if defined(Q_OS_WIN)
  v2RayExecFilePath    = QDir(v2RayInstallFolderPath).filePath("clash.exe");
#elif defined(Q_OS_LINUX) or defined(Q_OS_MAC)
  v2RayExecFilePath    = QDir(v2RayInstallFolderPath).filePath("clash");
#endif
  QDir v2RayInstallFolder(v2RayInstallFolderPath);
  // Create the install folder if not exists
  if (!v2RayInstallFolder.exists()) {
    v2RayInstallFolder.mkpath(".");
  }
  // Initialize QProcess
  v2rayProcess = new QProcess();
}

V2RayCore& V2RayCore::getInstance() {
  static V2RayCore v2RayCoreInstance;
  return v2RayCoreInstance;
}

V2RayCore::~V2RayCore() {
  this->stop();
  delete v2rayProcess;
}

QString V2RayCore::getVersion() {
  if (!isInstalled()) {
    return tr("Not Installed");
  }
  QProcess _v2rayProcess;
  _v2rayProcess.start(v2RayExecFilePath, {"-v"});
  _v2rayProcess.waitForFinished();
  QString v2RayVersion = _v2rayProcess.readAllStandardOutput();
  if (v2RayVersion.isEmpty()) {
    return tr("Unknown");
  }
  int sIndex = v2RayVersion.indexOf('v');
  int pIndex = v2RayVersion.indexOf(' ', sIndex);
  return v2RayVersion.mid(sIndex + 1, pIndex - sIndex - 1);
}

bool V2RayCore::start() {
  if (!isInstalled()) {
    return false;
  }
  // Get latest configuration for Clash
  Configurator& configurator(Configurator::getInstance());
  QJsonObject v2RayConfig = configurator.getV2RayConfig();
  QString configFilePath  = Configurator::getV2RayConfigFilePath();
  QFile configFile(Configurator::getV2RayConfigFilePath());
  // TODO: JSON -> YAML
  // configFile.open(QFile::WriteOnly);
  // configFile.write();
  // configFile.flush();

  // Start Clash
  QStringList arguments;
  QFileInfo configFileInfo(configFilePath);
  arguments << "-d" << configFileInfo.dir().absolutePath();
  v2rayProcess->start(v2RayExecFilePath, arguments);
  v2rayProcess->waitForFinished(500);
  int exitCode = v2rayProcess->exitCode();
  if (exitCode != 0) {
    qCritical() << "Failed to start Clash.";
    qCritical() << v2rayProcess->readAll();
  }
  // TODO: Fix Log Redirection
  v2rayProcess->setStandardErrorFile(Configurator::getV2RayLogFilePath());
  v2rayProcess->setStandardOutputFile(Configurator::getV2RayLogFilePath());
  return exitCode == 0;
}

bool V2RayCore::stop() {
  v2rayProcess->kill();
  v2rayProcess->waitForFinished();
  return v2rayProcess->state() == QProcess::NotRunning;
}

bool V2RayCore::restart() {
  stop();
  start();
  return isRunning();
}

bool V2RayCore::isRunning() {
  return v2rayProcess->state() == QProcess::Running;
}

bool V2RayCore::isInstalled() { return QFile(v2RayExecFilePath).exists(); }

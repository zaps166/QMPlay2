/*
 * This software uses code of <a href=http://ffmpeg.org>FFmpeg</a> licensed under the
 * <a href=http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>LGPLv2.1</a> and its
 * source can be downloaded <a href=link_to_your_sources>here</a>
 * */

#pragma once

#include <QWidget>
#include <QFileSystemWatcher>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QtWidgets/QTextEdit>
#include <QtCore/QProcess>

class QPlainTextEdit;
class QPushButton;

class ConvertWidget final : public QWidget
{
Q_OBJECT
public:
    ConvertWidget();
private:
    QLineEdit *videoLinePath;
    QLineEdit *subsLinePath;
    QLineEdit *outputLinePath;
    QLineEdit *outputFileName;
    QComboBox *outputFileType;
    QVBoxLayout *window;
    QTextEdit *executionText;
    QGroupBox *subsBox;
    QPushButton *convertB;


    void aboutArea();
    void videoArea();
    void subsArea();
    void outputArea();
    void buttonsArea();
    void executionArea();
private slots:
    void browseVideo();
    void browseSubs();
    void browseOutput();
    void convert();
    void cancel();
    void closeEvent(QCloseEvent *event);
};
/*
 * This software uses code of <a href=http://ffmpeg.org>FFmpeg</a> licensed under the
 * <a href=http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>LGPLv2.1</a> and its
 * source can be downloaded <a href=link_to_your_sources>here</a>
 * */

#include "ConvertWidget.hpp"

#include <Functions.hpp>
#include <Main.hpp>

#include <QUrl>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QLineEdit>
#include <QLayout>
#include <QComboBox>
#include <string>
#include <QMessageBox>

#ifdef linux
#include <stdlib.h>
#endif

/*
 * Constructor creates the window and shows the areas.*/
ConvertWidget::ConvertWidget()
{
    setWindowTitle(tr("Convert"));
    setAttribute(Qt::WA_DeleteOnClose);

    window = new QVBoxLayout(this);
    videoLinePath = new QLineEdit;
    videoLinePath->setText(tr(""));
    subsLinePath = new QLineEdit;
    subsLinePath->setText(tr(""));
    outputLinePath = new QLineEdit;
    outputLinePath->setText("");
    outputFileName = new QLineEdit;
    outputFileName->setText("");

    aboutArea();
    videoArea();
    subsArea();
    outputArea();
    buttonsArea();
    window->setSpacing(0);
    setLayout(window);

    show();

}

/*
 * The SLOT opens the file manager in order to give the chance to user to choose a video file.
 * */
void ConvertWidget::browseVideo()
{
    QString videoPath = QFileDialog::getOpenFileName(this, tr("Select a video file"), "directoryToOpen",tr( "*.mp4;;*.avi"));
    videoLinePath->setText(videoPath);
}

/*
 * The SLOT opens the file manager in order to give the chance to user to choose a subtitle file.
 * */
void ConvertWidget::browseSubs()
{
    QString subsPath = QFileDialog::getOpenFileName(this, tr("Select a subtitle file"), "directoryToOpen",tr( "*.srt"));
    subsLinePath->setText(subsPath);
}

/*
 * The SLOT opens the file manager in order to give the chance to user to choose a directory to save the new video file.
 * */
void ConvertWidget::browseOutput()
{
    QString outputPath = QFileDialog::getExistingDirectory(this, "Where do you want to save the file");
    outputLinePath->setText(outputPath);
}

/*
 * Works ONLY in LINUX.
 * The SLOT checks if the areas are empty. If they are empty then pop-up a warning. If they are not empty then convert the video file.
 * */
void ConvertWidget::convert()
{
    // Print a pop-up window if the video path and/or output path and/or output name is empty.
    if (videoLinePath->text().isEmpty() || subsLinePath->text().isEmpty() || outputLinePath->text().isEmpty() || outputFileName->text().isEmpty())
    {
        QMessageBox::warning(
                this,
                tr("Convert Warning"),
                tr("Video File Selection and/or Subtitle File Selection and/or Export File Selection are/is empty.") );
        return;
    }
    // this code works ONLY on LINUX
#ifdef linux
    std::string command;

    QString video = videoLinePath->text();
    QString outputPath = outputLinePath->text();
    QString outputName = outputFileName->text();
    QString outputType = outputFileType->currentText();
    QString outputFile = outputPath + "/" + outputName + outputType;

    QString subs = subsLinePath->text();
    std::string newSubs = subs.toUtf8().constData();
    newSubs = newSubs.substr(0, newSubs.length()-4) + ".ass";

    // merge the video file with the subtitle file
    // opens a terminal window and runs the command: ffmpeg -i subtitleFile subtitle.ass && ffmpeg -i videoFile -vf ass=subtitle.ass newVideoFile && rm subtitle.ass && exit
    // subtitleFile is the path of the subtitle file that the user has choose
    // videoFile is the path of the video file that the user has choose
    // newVideoFile is the path of the newVideo file that the user has choose
    command = "x-terminal-emulator -e 'sh -c \"ffmpeg -i ";
    command = command + subs.toUtf8().constData() + " ";
    command = command + newSubs + " && ffmpeg -i ";
    command = command + video.toUtf8().constData() + " -vf ass=";
    command = command + newSubs + " ";
    command = command + outputFile.toUtf8().constData() + " && rm ";
    command = command + newSubs + " && exit\"'";
    system(command.c_str());
#endif
    close();
}

/*
 * This function creates a label.
 * */
void ConvertWidget::aboutArea()
{
    QLabel *about = new QLabel(tr("This is a plugin that can merge a video file with \na subtitle file and "
                                  "change the type of a video file. \n"
                                  "This software uses libraries from the FFmpeg project under the LGPLv2.1"
                                  " \n\nImportant: Paths and names can not "
                                  "contain spaces!\nImportant: Works only for LINUX!\n"));
    window->addWidget(about);
}

/*
 * This function creates a group box with a button and a line edit.
 * */
void ConvertWidget::videoArea()
{
    QPushButton *videoB = new QPushButton;
    videoB->setText("Browse");
    connect(videoB, SIGNAL(clicked()), this, SLOT(browseVideo()));

    QGroupBox *videoBox = new QGroupBox(tr("Video File Selection"));
    QGridLayout *videoLayout = new QGridLayout;
    videoLayout->addWidget(videoLinePath, 0, 0);
    videoLayout->addWidget(videoB, 0, 1);
    videoBox->setLayout(videoLayout);
    window->addWidget(videoBox);
}

/*
 * This function creates a group box with a button and a line edit.
 * */
void ConvertWidget::subsArea()
{
    QPushButton *subsB = new QPushButton;
    subsB->setText("Browse");
    connect(subsB, SIGNAL(clicked()), this, SLOT(browseSubs()));

    subsBox = new QGroupBox(tr("Subtitle File Selection"));
    subsBox->setChecked(false);
    QGridLayout *subsLayout = new QGridLayout;
    subsLayout->addWidget(subsLinePath, 0, 0);
    subsLayout->addWidget(subsB, 0, 1);
    subsBox->setLayout(subsLayout);
    window->addWidget(subsBox);

}

/*
 * This function creates a group box with buttons, lines edit, labels and combo box.
 * */
void ConvertWidget::outputArea()
{
    QPushButton *outputB = new QPushButton;
    outputB->setText("Browse");
    connect(outputB, SIGNAL(clicked()), this, SLOT(browseOutput()));

    QGroupBox *outputBox = new QGroupBox(tr("Export File Selection"));
    QGridLayout *outputLayout = new QGridLayout;
    QLabel *outputPath = new QLabel(tr("Where do you want to save the new file:"));
    outputLayout->addWidget(outputPath, 0, 0);
    outputLayout->addWidget(outputLinePath, 0, 1);
    outputLayout->addWidget(outputB, 0, 2);
    QLabel *outputName = new QLabel(tr("Give a name to the new file and select the type:"));
    outputLayout->addWidget(outputName, 1, 0);
    outputLayout->addWidget(outputFileName, 1, 1);
    outputFileType = new QComboBox;
    outputFileType->addItem(tr(".mp4"));
    outputFileType->addItem(tr(".avi"));
    outputLayout->addWidget(outputFileType, 1, 2);
    outputBox->setLayout(outputLayout);
    window->addWidget(outputBox);
}

/*
 * This function creates a group box with buttons.
 * */
void ConvertWidget::buttonsArea()
{
    QGroupBox *buttonsBox = new QGroupBox(tr("Select"));
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    QPushButton *convertB = new QPushButton;
    convertB->setText(tr("Convert"));
    buttonsLayout->addWidget(convertB);
    QPushButton *cancelB = new QPushButton;
    cancelB->setText(tr("Cancel"));
    cancelB->setShortcut(QKeySequence("ESC"));
    buttonsLayout->addWidget(cancelB);
    buttonsBox->setLayout(buttonsLayout);
    buttonsBox->setContentsMargins(0, 20, 0, 0);
    window->addWidget(buttonsBox);

    connect(cancelB, SIGNAL(clicked()), this, SLOT(close()));
    connect(convertB, SIGNAL(clicked()), this, SLOT(convert()));
}

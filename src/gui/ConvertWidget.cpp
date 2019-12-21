//
// Created by dimitris on 21/12/19.
//

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
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define DIV 1048576
#define WIDTH 7
#endif

#ifdef linux
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

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
    outputLinePath->setText("");

    videoArea();
    subsArea();
    outputArea();
    buttonsArea();
    window->setSpacing(0);
    setLayout(window);

    show();

}

void ConvertWidget::browseVideo()
{
    QString videoPath = QFileDialog::getOpenFileName(this, tr("Select a video file"), "directoryToOpen",tr( "*.mp4;;*.avi"));
    videoLinePath->setText(videoPath);
}

void ConvertWidget::browseSubs()
{
    QString subsPath = QFileDialog::getOpenFileName(this, tr("Select a subtitle file"), "directoryToOpen",tr( "*.srt;;*.ass"));
    subsLinePath->setText(subsPath);
}

void ConvertWidget::browseOutput()
{
    QString outputPath = QFileDialog::getExistingDirectory(this, "Where do you want to save the file");
    outputLinePath->setText(outputPath);
}

void ConvertWidget::convert()
{
#ifdef _WIN32

#endif

#ifdef linux
    QString video = videoLinePath->text();
    QString subs = subsLinePath->text();
    std::string newSubs = subs.toUtf8().constData();
    newSubs = newSubs.substr(0, newSubs.length()-4)+".ass";
    QString outputPath = outputLinePath->text();
    QString outputName = outputFileName->text();
    std::string command;
    command = "gnome-terminal -e 'sh -c \"ffmpeg -i ";
    command = command+subs.toUtf8().constData()+" ";
    command = command+newSubs+" && ffmpeg -i ";
    command = command+video.toUtf8().constData()+" -vf ass=";
    command = command+newSubs+" ";
    command = command+outputPath.toUtf8().constData()+"/";
    command = command+outputName.toUtf8().constData()+".mp4\"'";
    std::cout<<command<<std::endl;
    system(command.c_str());
#endif
}

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

void ConvertWidget::subsArea()
{
    QPushButton *subsB = new QPushButton;
    subsB->setText("Browse");
    connect(subsB, SIGNAL(clicked()), this, SLOT(browseSubs()));

    QGroupBox *subsBox = new QGroupBox(tr("Do you want to use a subtitle file?"));
    subsBox->setCheckable(true);
    subsBox->setChecked(false);
    QGridLayout *subsLayout = new QGridLayout;
    subsLayout->addWidget(subsLinePath, 0, 0);
    subsLayout->addWidget(subsB, 0, 1);
    subsBox->setLayout(subsLayout);
    window->addWidget(subsBox);

}

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
    QComboBox *outputComboBox = new QComboBox;
    outputComboBox->addItem(tr(".mp4"));
    //outputComboBox->addItem(tr(".avi"));
    outputLayout->addWidget(outputComboBox, 1, 2);
    outputBox->setLayout(outputLayout);
    window->addWidget(outputBox);
}

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
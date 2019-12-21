//
// Created by dimitris on 21/12/19.
//

#pragma once

#include <QWidget>
#include <QFileSystemWatcher>
#include <QLineEdit>
#include <QVBoxLayout>

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
    QVBoxLayout *window;

    void videoArea();
    void subsArea();
    void outputArea();
    void buttonsArea();
private slots:
    void browseVideo();
    void browseSubs();
    void browseOutput();
    void convert();
};
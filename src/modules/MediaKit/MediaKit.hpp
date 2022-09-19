#pragma once

#include <Module.hpp>

class MediaKit : public Module
{
public:
    MediaKit();
private:
    QList< Info > getModulesInfo( const bool ) const;
    void *createInstance( const QString & );

    SettingsWidget *getSettingsWidget();
};

/**/

#include <QCoreApplication>

class QDoubleSpinBox;
class QCheckBox;

class ModuleSettingsWidget : public Module::SettingsWidget
{
    Q_DECLARE_TR_FUNCTIONS( ModuleSettingsWidget )
public:
    ModuleSettingsWidget( Module & );
private:
    void saveSettings();

    QCheckBox *enabledB;
    QDoubleSpinBox *delayB;
};

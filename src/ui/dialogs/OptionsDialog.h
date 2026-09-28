// Preferencias de la aplicacion: idioma, tema, iconos, carpeta por defecto de
// extraccion, instancia unica y comportamiento al abrir un archivo.
#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;

namespace qtrar {

class OptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionsDialog(QWidget *parent = nullptr);

    /// Aplica lo elegido a QSettings y deja tema e iconos puestos al momento.
    /// Devuelve true si el idioma ha cambiado, que es lo unico que exige
    /// reiniciar: las traducciones se cargan una vez al arrancar.
    bool apply();

private:
    /// Valores de partida, leidos de QSettings.
    void load();

    QComboBox *m_languageCombo;
    QComboBox *m_themeCombo;
    QComboBox *m_iconsCombo;
    QLineEdit *m_extractPathEdit;
    QCheckBox *m_singleInstanceCheck;
    QCheckBox *m_confirmOverwriteCheck;
    QCheckBox *m_keepGeometryCheck;
    QCheckBox *m_createVolumesCheck;
    QSpinBox *m_createLevelSpin;

    QString m_originalLanguage;
};

} // namespace qtrar

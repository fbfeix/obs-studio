#pragma once

#include <utility/vertical/VerticalModel.hpp>

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;

/* Edits the profile's vertical stream destination. The stream key stays in
 * the profile directory and is masked by default. */
class VerticalStreamDialog : public QDialog {
	Q_OBJECT

public:
	VerticalStreamDialog(const OBS::Vertical::VerticalStreamSettings &settings, QWidget *parent = nullptr);

	[[nodiscard]] OBS::Vertical::VerticalStreamSettings Settings() const;

public slots:
	void accept() override;

private:
	QCheckBox *enabled = nullptr;
	QLineEdit *server = nullptr;
	QLineEdit *key = nullptr;
	QSpinBox *bitrate = nullptr;
	QLabel *error = nullptr;
};

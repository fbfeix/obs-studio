#include "VerticalStreamDialog.hpp"

#include <OBSApp.hpp>

#include <qt-wrappers.hpp>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

using OBS::Vertical::StreamSettingsError;
using OBS::Vertical::VerticalStreamSettings;

namespace {
QString ErrorText(StreamSettingsError error)
{
	switch (error) {
	case StreamSettingsError::MissingServer:
		return QTStr("Basic.Vertical.Stream.Error.MissingServer");
	case StreamSettingsError::UnsupportedProtocol:
		return QTStr("Basic.Vertical.Stream.Error.UnsupportedProtocol");
	case StreamSettingsError::MissingKey:
		return QTStr("Basic.Vertical.Stream.Error.MissingKey");
	case StreamSettingsError::None:
		break;
	}
	return {};
}
} // namespace

VerticalStreamDialog::VerticalStreamDialog(const VerticalStreamSettings &settings, QWidget *parent) : QDialog(parent)
{
	setWindowTitle(QTStr("Basic.Vertical.Stream.Title"));
	setMinimumWidth(520);

	enabled = new QCheckBox(QTStr("Basic.Vertical.Stream.Enabled"), this);
	enabled->setChecked(settings.enabled);

	server = new QLineEdit(QT_UTF8(settings.server.c_str()), this);
	server->setPlaceholderText("rtmps://…");

	key = new QLineEdit(QT_UTF8(settings.key.c_str()), this);
	key->setEchoMode(QLineEdit::Password);
	auto *showKey = new QPushButton(QTStr("Show"), this);
	showKey->setCheckable(true);
	connect(showKey, &QPushButton::toggled, this, [this, showKey](bool visible) {
		key->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
		showKey->setText(visible ? QTStr("Hide") : QTStr("Show"));
	});
	auto *keyRow = new QHBoxLayout();
	keyRow->addWidget(key);
	keyRow->addWidget(showKey);

	bitrate = new QSpinBox(this);
	bitrate->setRange(VerticalStreamSettings::kMinBitrate, VerticalStreamSettings::kMaxBitrate);
	bitrate->setSingleStep(500);
	bitrate->setSuffix(" kbps");
	bitrate->setValue(settings.videoBitrate);

	auto *hint = new QLabel(QTStr("Basic.Vertical.Stream.Hint"), this);
	hint->setWordWrap(true);
	hint->setTextInteractionFlags(Qt::TextSelectableByMouse);

	error = new QLabel(this);
	error->setWordWrap(true);
	error->setStyleSheet("color: #e05050;");
	error->hide();

	auto *form = new QFormLayout();
	form->addRow(enabled);
	form->addRow(QTStr("Basic.Vertical.Stream.Server"), server);
	form->addRow(QTStr("Basic.AutoConfig.StreamPage.StreamKey"), keyRow);
	form->addRow(QTStr("Basic.Settings.Output.VideoBitrate"), bitrate);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &VerticalStreamDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &VerticalStreamDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(hint);
	layout->addWidget(error);
	layout->addWidget(buttons);
}

VerticalStreamSettings VerticalStreamDialog::Settings() const
{
	OBSDataAutoRelease data = obs_data_create();
	obs_data_set_bool(data, "enabled", enabled->isChecked());
	obs_data_set_string(data, "server", QT_TO_UTF8(server->text()));
	obs_data_set_string(data, "key", QT_TO_UTF8(key->text()));
	obs_data_set_int(data, "video_bitrate", bitrate->value());
	/* Same normalization (trimming, range checks) as loading from disk. */
	return VerticalStreamSettings::FromData(data);
}

void VerticalStreamDialog::accept()
{
	VerticalStreamSettings settings = Settings();
	bool emptyAndOff = !settings.enabled && settings.server.empty() && settings.key.empty();
	StreamSettingsError validation = settings.Validate();

	if (!emptyAndOff && validation != StreamSettingsError::None) {
		error->setText(ErrorText(validation));
		error->show();
		return;
	}
	QDialog::accept();
}

#include "VerticalCanvasDock.hpp"

#include <OBSApp.hpp>
#include <utility/display-helpers.hpp>
#include <widgets/OBSQTDisplay.hpp>

#include <qt-wrappers.hpp>

#include <graphics/vec2.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kItemIdRole = Qt::UserRole;
constexpr int kSceneUuidRole = Qt::UserRole + 1;

QToolButton *MakeButton(const QString &text, const QString &tooltip, QWidget *parent)
{
	auto *button = new QToolButton(parent);
	button->setText(text);
	button->setToolTip(tooltip);
	button->setAutoRaise(true);
	return button;
}
} // namespace

VerticalCanvasDock::VerticalCanvasDock(OBS::Vertical::VerticalCanvas &vertical_, QWidget *parent)
	: QWidget(parent),
	  vertical(vertical_)
{
	preview = new OBSQTDisplay(this);
	preview->setMinimumSize(144, 256);
	preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	connect(preview, &OBSQTDisplay::DisplayCreated, this, [this](OBSQTDisplay *display) {
		obs_display_add_draw_callback(display->GetDisplay(), VerticalCanvasDock::RenderPreview, this);
	});

	sceneList = new QListWidget(this);
	sceneList->setMaximumHeight(110);
	connect(sceneList, &QListWidget::itemSelectionChanged, this, &VerticalCanvasDock::SceneSelected);

	auto *sceneButtons = new QHBoxLayout();
	auto *addScene = MakeButton("+", QTStr("Basic.Vertical.AddScene"), this);
	auto *removeScene = MakeButton("−", QTStr("Basic.Vertical.RemoveScene"), this);
	connect(addScene, &QToolButton::clicked, this, &VerticalCanvasDock::AddScene);
	connect(removeScene, &QToolButton::clicked, this, &VerticalCanvasDock::RemoveScene);
	sceneButtons->addWidget(addScene);
	sceneButtons->addWidget(removeScene);
	sceneButtons->addStretch();

	pairLabel = new QLabel(this);
	pairLabel->setWordWrap(true);
	pairCombo = new QComboBox(this);
	pairCombo->setToolTip(QTStr("Basic.Vertical.PairTooltip"));
	connect(pairCombo, &QComboBox::currentIndexChanged, this, &VerticalCanvasDock::PairChanged);

	itemList = new QListWidget(this);
	connect(itemList, &QListWidget::itemChanged, this, &VerticalCanvasDock::ItemChanged);

	auto *itemButtons = new QHBoxLayout();
	auto *addSource = MakeButton("+", QTStr("Basic.Vertical.AddExistingSource"), this);
	auto *removeItem = MakeButton("−", QTStr("Basic.Vertical.RemoveSource"), this);
	auto *moveUp = MakeButton("↑", QTStr("Basic.Vertical.MoveUp"), this);
	auto *moveDown = MakeButton("↓", QTStr("Basic.Vertical.MoveDown"), this);
	auto *fit = MakeButton(QTStr("Basic.Vertical.Fit"), QTStr("Basic.Vertical.FitTooltip"), this);
	auto *fill = MakeButton(QTStr("Basic.Vertical.Fill"), QTStr("Basic.Vertical.FillTooltip"), this);
	auto *center = MakeButton(QTStr("Basic.Vertical.Center"), QTStr("Basic.Vertical.CenterTooltip"), this);
	connect(addSource, &QToolButton::clicked, this, &VerticalCanvasDock::ShowAddSourceMenu);
	connect(removeItem, &QToolButton::clicked, this, &VerticalCanvasDock::RemoveItem);
	connect(moveUp, &QToolButton::clicked, this, &VerticalCanvasDock::MoveItemUp);
	connect(moveDown, &QToolButton::clicked, this, &VerticalCanvasDock::MoveItemDown);
	connect(fit, &QToolButton::clicked, this, &VerticalCanvasDock::FitItem);
	connect(fill, &QToolButton::clicked, this, &VerticalCanvasDock::FillItem);
	connect(center, &QToolButton::clicked, this, &VerticalCanvasDock::CenterItem);
	for (auto *button : {addSource, removeItem, moveUp, moveDown, fit, fill, center}) {
		itemButtons->addWidget(button);
	}
	itemButtons->addStretch();

	auto *streamRow = new QHBoxLayout();
	streamStatus = new QLabel(this);
	streamStatus->setWordWrap(true);
	auto *streamSettings = new QPushButton(QTStr("Basic.Vertical.Stream.Button"), this);
	connect(streamSettings, &QPushButton::clicked, this, [this]() {
		if (openStreamSettings) {
			openStreamSettings();
		}
	});
	streamRow->addWidget(streamStatus, 1);
	streamRow->addWidget(streamSettings);
	SetStreamStatus(QTStr("Basic.Vertical.Stream.Status.Off"), false);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->addWidget(preview, 3);
	layout->addWidget(sceneList);
	layout->addLayout(sceneButtons);
	layout->addWidget(pairLabel);
	layout->addWidget(pairCombo);
	layout->addWidget(itemList, 2);
	layout->addLayout(itemButtons);
	layout->addLayout(streamRow);

	vertical.SetChangedCallback(
		[this]() { QMetaObject::invokeMethod(this, "RefreshScenes", Qt::QueuedConnection); });
	RefreshScenes();
}

VerticalCanvasDock::~VerticalCanvasDock()
{
	vertical.SetChangedCallback(nullptr);
	sceneSignals.clear();
	if (preview && preview->GetDisplay()) {
		obs_display_remove_draw_callback(preview->GetDisplay(), VerticalCanvasDock::RenderPreview, this);
	}
}

void VerticalCanvasDock::RenderPreview(void *data, uint32_t cx, uint32_t cy)
{
	auto *dock = static_cast<VerticalCanvasDock *>(data);
	OBSCanvas canvas;
	{
		std::lock_guard lock(dock->previewMutex);
		canvas = OBSGetStrongRef(dock->previewCanvas);
	}
	obs_video_info ovi = {};
	if (!canvas || !obs_canvas_get_video_info(canvas, &ovi)) {
		return;
	}

	int x, y, newCX, newCY;
	float scale;
	GetScaleAndCenterPos(int(ovi.base_width), int(ovi.base_height), int(cx), int(cy), x, y, scale);
	newCX = int(scale * float(ovi.base_width));
	newCY = int(scale * float(ovi.base_height));

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);
	obs_render_canvas_texture(canvas);
	gs_projection_pop();
	gs_viewport_pop();
}

void VerticalCanvasDock::SceneItemsChanged(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<VerticalCanvasDock *>(data), "RefreshItems", Qt::QueuedConnection);
}

void VerticalCanvasDock::RefreshScenes()
{
	OBSSource current = vertical.GetCurrentScene();
	{
		OBSCanvas canvas = vertical.GetCanvas();
		std::lock_guard lock(previewMutex);
		previewCanvas = canvas ? OBSGetWeakRef(canvas) : OBSWeakCanvas();
	}

	refreshing = true;
	sceneList->clear();
	for (const auto &scene : vertical.GetScenes()) {
		auto *listItem = new QListWidgetItem(QT_UTF8(obs_source_get_name(scene)), sceneList);
		listItem->setData(kSceneUuidRole, QT_UTF8(obs_source_get_uuid(scene)));
		if (scene.Get() == current.Get()) {
			listItem->setSelected(true);
		}
	}
	refreshing = false;

	ConnectSceneSignals(current);
	RefreshPair();
	RefreshItems();
}

void VerticalCanvasDock::SetStreamStatus(const QString &status, bool error)
{
	streamStatus->setText(status);
	streamStatus->setStyleSheet(error ? "color: #e05050;" : "");
}

void VerticalCanvasDock::RefreshPair()
{
	OBSSource mainScene = vertical.GetMainScene();
	OBSSource paired = vertical.GetPairedScene(mainScene);

	refreshing = true;
	pairCombo->clear();
	pairCombo->setEnabled(mainScene != nullptr);
	pairLabel->setText(mainScene ? QTStr("Basic.Vertical.PairedWith").arg(QT_UTF8(obs_source_get_name(mainScene)))
				     : QTStr("Basic.Vertical.NoMainScene"));

	pairCombo->addItem(QTStr("Basic.Vertical.NotPaired"), QString());
	for (const auto &scene : vertical.GetScenes()) {
		pairCombo->addItem(QT_UTF8(obs_source_get_name(scene)), QT_UTF8(obs_source_get_uuid(scene)));
		if (paired && scene.Get() == paired.Get()) {
			pairCombo->setCurrentIndex(pairCombo->count() - 1);
		}
	}
	refreshing = false;
}

void VerticalCanvasDock::PairChanged(int index)
{
	if (refreshing || index < 0) {
		return;
	}

	OBSSource mainScene = vertical.GetMainScene();
	if (!mainScene) {
		return;
	}

	QByteArray uuid = pairCombo->itemData(index).toString().toUtf8();
	OBSSourceAutoRelease verticalScene = uuid.isEmpty() ? nullptr : obs_get_source_by_uuid(uuid.constData());
	vertical.SetPair(mainScene, verticalScene);
}

void VerticalCanvasDock::ConnectSceneSignals(obs_source_t *scene)
{
	sceneSignals.clear();
	if (!scene) {
		return;
	}

	signal_handler_t *handler = obs_source_get_signal_handler(scene);
	for (const char *signal : {"item_add", "item_remove", "reorder", "item_visible", "refresh"}) {
		sceneSignals.emplace_back(handler, signal, VerticalCanvasDock::SceneItemsChanged, this);
	}
}

void VerticalCanvasDock::RefreshItems()
{
	OBSScene scene = CurrentScene();
	int64_t selectedId = -1;
	if (QListWidgetItem *selected = itemList->currentItem()) {
		selectedId = selected->data(kItemIdRole).toLongLong();
	}

	refreshing = true;
	itemList->clear();
	if (scene) {
		std::vector<OBSSceneItem> items;
		auto cb = [](obs_scene_t *, obs_sceneitem_t *item, void *param) {
			static_cast<std::vector<OBSSceneItem> *>(param)->emplace_back(item);
			return true;
		};
		obs_scene_enum_items(scene, cb, &items);

		/* Top-most item first, like the main sources list. */
		std::reverse(items.begin(), items.end());
		for (const auto &item : items) {
			obs_source_t *source = obs_sceneitem_get_source(item);
			auto *listItem = new QListWidgetItem(QT_UTF8(obs_source_get_name(source)), itemList);
			listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
			listItem->setCheckState(obs_sceneitem_visible(item) ? Qt::Checked : Qt::Unchecked);
			listItem->setData(kItemIdRole, QVariant::fromValue<qlonglong>(obs_sceneitem_get_id(item)));
			if (obs_sceneitem_get_id(item) == selectedId) {
				itemList->setCurrentItem(listItem);
			}
		}
	}
	refreshing = false;
}

OBSScene VerticalCanvasDock::CurrentScene() const
{
	return obs_scene_from_source(vertical.GetCurrentScene());
}

OBSSceneItem VerticalCanvasDock::SelectedItem() const
{
	OBSScene scene = CurrentScene();
	QListWidgetItem *listItem = itemList->currentItem();
	if (!scene || !listItem) {
		return nullptr;
	}
	return obs_scene_find_sceneitem_by_id(scene, listItem->data(kItemIdRole).toLongLong());
}

void VerticalCanvasDock::AddScene()
{
	vertical.CreateScene(OBS::Vertical::VerticalCanvas::kDefaultSceneName);
}

void VerticalCanvasDock::RemoveScene()
{
	vertical.RemoveScene(vertical.GetCurrentScene());
}

void VerticalCanvasDock::SceneSelected()
{
	if (refreshing) {
		return;
	}

	QList<QListWidgetItem *> selected = sceneList->selectedItems();
	if (selected.isEmpty()) {
		return;
	}

	QByteArray uuid = selected.first()->data(kSceneUuidRole).toString().toUtf8();
	OBSSourceAutoRelease scene = obs_get_source_by_uuid(uuid.constData());
	if (scene && scene.Get() != vertical.GetCurrentScene().Get()) {
		vertical.SetCurrentScene(scene);
	}
}

void VerticalCanvasDock::ShowAddSourceMenu()
{
	OBSScene scene = CurrentScene();
	if (!scene) {
		return;
	}

	std::vector<OBSSource> sources;
	auto cb = [](void *param, obs_source_t *source) {
		if (obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) {
			static_cast<std::vector<OBSSource> *>(param)->emplace_back(source);
		}
		return true;
	};
	obs_enum_sources(cb, &sources);
	std::sort(sources.begin(), sources.end(), [](const OBSSource &a, const OBSSource &b) {
		return QString::localeAwareCompare(QT_UTF8(obs_source_get_name(a)), QT_UTF8(obs_source_get_name(b))) <
		       0;
	});

	QMenu menu(this);
	for (const auto &source : sources) {
		QAction *action = menu.addAction(QT_UTF8(obs_source_get_name(source)));
		connect(action, &QAction::triggered, this, [scene, source]() {
			obs_sceneitem_t *item = obs_scene_add(scene, source);
			if (item) {
				obs_sceneitem_set_visible(item, true);
			}
		});
	}
	if (sources.empty()) {
		menu.addAction(QTStr("Basic.Vertical.NoSources"))->setEnabled(false);
	}
	menu.exec(QCursor::pos());
}

void VerticalCanvasDock::RemoveItem()
{
	if (OBSSceneItem item = SelectedItem()) {
		obs_sceneitem_remove(item);
	}
}

void VerticalCanvasDock::MoveItemUp()
{
	if (OBSSceneItem item = SelectedItem()) {
		obs_sceneitem_set_order(item, OBS_ORDER_MOVE_UP);
	}
}

void VerticalCanvasDock::MoveItemDown()
{
	if (OBSSceneItem item = SelectedItem()) {
		obs_sceneitem_set_order(item, OBS_ORDER_MOVE_DOWN);
	}
}

void VerticalCanvasDock::ApplyBounds(obs_bounds_type type)
{
	OBSSceneItem item = SelectedItem();
	OBSCanvas canvas = vertical.GetCanvas();
	obs_video_info ovi = {};
	if (!item || !canvas || !obs_canvas_get_video_info(canvas, &ovi)) {
		return;
	}

	vec2 pos = {0.0f, 0.0f};
	vec2 bounds = {float(ovi.base_width), float(ovi.base_height)};
	obs_sceneitem_set_alignment(item, OBS_ALIGN_LEFT | OBS_ALIGN_TOP);
	obs_sceneitem_set_bounds_type(item, type);
	obs_sceneitem_set_bounds_alignment(item, OBS_ALIGN_CENTER);
	obs_sceneitem_set_bounds(item, &bounds);
	obs_sceneitem_set_pos(item, &pos);
}

void VerticalCanvasDock::FitItem()
{
	ApplyBounds(OBS_BOUNDS_SCALE_INNER);
}

void VerticalCanvasDock::FillItem()
{
	ApplyBounds(OBS_BOUNDS_SCALE_OUTER);
}

void VerticalCanvasDock::CenterItem()
{
	OBSSceneItem item = SelectedItem();
	OBSCanvas canvas = vertical.GetCanvas();
	obs_video_info ovi = {};
	if (!item || !canvas || !obs_canvas_get_video_info(canvas, &ovi)) {
		return;
	}

	vec2 pos = {float(ovi.base_width) / 2.0f, float(ovi.base_height) / 2.0f};
	obs_sceneitem_set_alignment(item, OBS_ALIGN_CENTER);
	obs_sceneitem_set_pos(item, &pos);
}

void VerticalCanvasDock::ItemChanged(QListWidgetItem *listItem)
{
	if (refreshing || !listItem) {
		return;
	}

	OBSScene scene = CurrentScene();
	if (!scene) {
		return;
	}

	OBSSceneItem item = obs_scene_find_sceneitem_by_id(scene, listItem->data(kItemIdRole).toLongLong());
	if (item) {
		obs_sceneitem_set_visible(item, listItem->checkState() == Qt::Checked);
	}
}

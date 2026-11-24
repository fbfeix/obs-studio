#pragma once

#include <utility/vertical/VerticalCanvas.hpp>

#include <obs.hpp>

#include <QWidget>

#include <functional>
#include <mutex>
#include <vector>

class OBSQTDisplay;
class QComboBox;
class QLabel;
class QListWidget;
class QListWidgetItem;

/* Dock content for the native vertical canvas: a live preview, the canvas'
 * scene list and the items of the current vertical scene. */
class VerticalCanvasDock : public QWidget {
	Q_OBJECT

public:
	explicit VerticalCanvasDock(OBS::Vertical::VerticalCanvas &vertical, QWidget *parent = nullptr);
	~VerticalCanvasDock() override;

	void SetStreamSettingsHandler(std::function<void()> handler) { openStreamSettings = std::move(handler); }

public slots:
	void RefreshScenes();
	void SetStreamStatus(const QString &status, bool error);
	void RefreshItems();

private slots:
	void AddScene();
	void RemoveScene();
	void SceneSelected();
	void ShowAddSourceMenu();
	void RemoveItem();
	void MoveItemUp();
	void MoveItemDown();
	void FitItem();
	void FillItem();
	void CenterItem();
	void ItemChanged(QListWidgetItem *listItem);
	void PairChanged(int index);

private:
	static void RenderPreview(void *data, uint32_t cx, uint32_t cy);
	static void SceneItemsChanged(void *data, calldata_t *params);

	[[nodiscard]] OBSScene CurrentScene() const;
	[[nodiscard]] OBSSceneItem SelectedItem() const;
	void ConnectSceneSignals(obs_source_t *scene);
	void ApplyBounds(obs_bounds_type type);
	void RefreshPair();

	OBS::Vertical::VerticalCanvas &vertical;
	OBSQTDisplay *preview = nullptr;
	QListWidget *sceneList = nullptr;
	QListWidget *itemList = nullptr;
	QLabel *pairLabel = nullptr;
	QComboBox *pairCombo = nullptr;
	QLabel *streamStatus = nullptr;
	std::function<void()> openStreamSettings;
	std::vector<OBSSignal> sceneSignals;
	bool refreshing = false;

	/* Read by the graphics thread while rendering the preview. */
	std::mutex previewMutex;
	OBSWeakCanvas previewCanvas;
};
